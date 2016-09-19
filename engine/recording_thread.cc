// Floating Temple
// Copyright 2015 Derek S. Snyder
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "engine/recording_thread.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "engine/committed_event.h"
#include "engine/live_object.h"
#include "engine/pending_transaction.h"
#include "engine/proto/transaction_id.pb.h"
#include "engine/recording_method_context.h"
#include "engine/transaction_id_util.h"
#include "engine/transaction_store_internal_interface.h"
#include "include/c++/value.h"

using std::shared_ptr;
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

namespace floating_temple {
namespace engine {
namespace {

void AddEventToMap(ObjectReferenceImpl* object_reference, CommittedEvent* event,
                   unordered_map<ObjectReferenceImpl*, vector<CommittedEvent*>>*
                       object_events) {
  CHECK(object_reference != nullptr);
  CHECK(event != nullptr);
  CHECK(object_events != nullptr);

  (*object_events)[object_reference].push_back(event);
}

}  // namespace

RecordingThread::RecordingThread(
    TransactionStoreInternalInterface* transaction_store)
    : transaction_store_(CHECK_NOTNULL(transaction_store)),
      pending_transaction_(
          new PendingTransaction(
              transaction_store, MIN_TRANSACTION_ID,
              transaction_store->GetCurrentSequencePoint())) {
}

RecordingThread::~RecordingThread() {
}

void RecordingThread::RunProgram(LocalObject* local_object,
                                 const string& method_name,
                                 Value* return_value,
                                 bool linger) {
  CHECK(return_value != nullptr);

  ObjectReferenceImpl* const object_reference = CreateObject(
      nullptr, shared_ptr<LiveObject>(nullptr), local_object, "");

  for (;;) {
    Value return_value_temp;
    if (CallMethod(nullptr, shared_ptr<LiveObject>(nullptr), object_reference,
                   method_name, vector<Value>(), &return_value_temp)) {
      if (!linger) {
        *return_value = return_value_temp;
        return;
      }

      // The program completed successfully. Enter linger mode. This allows
      // execution of the current thread to be rewound if another peer rejects a
      // transaction from this peer.
      transaction_store_->WaitForRewind();
    }
  }
}

bool RecordingThread::BeginTransaction(
    ObjectReferenceImpl* caller_object_reference,
    const shared_ptr<LiveObject>& caller_live_object) {
  VLOG(2) << "RecordingThread::BeginTransaction";

  if (Rewinding()) {
    return false;
  }

  if (caller_object_reference != nullptr) {
    AddTransactionEvent(caller_object_reference,
                        new BeginTransactionCommittedEvent(),
                        caller_object_reference, caller_live_object);
  }

  pending_transaction_->IncrementTransactionLevel();

  return true;
}

bool RecordingThread::EndTransaction(
    ObjectReferenceImpl* caller_object_reference,
    const shared_ptr<LiveObject>& caller_live_object) {
  VLOG(2) << "RecordingThread::EndTransaction";

  if (Rewinding()) {
    return false;
  }

  if (caller_object_reference != nullptr) {
    AddTransactionEvent(caller_object_reference,
                        new EndTransactionCommittedEvent(),
                        caller_object_reference, caller_live_object);
  }

  if (pending_transaction_->DecrementTransactionLevel()) {
    CommitTransaction();
  }

  return true;
}

ObjectReferenceImpl* RecordingThread::CreateObject(
    ObjectReferenceImpl* caller_object_reference,
    const shared_ptr<LiveObject>& caller_live_object,
    LocalObject* initial_version,
    const string& name) {
  // Take ownership of *initial_version.
  shared_ptr<const LiveObject> new_live_object(new LiveObject(initial_version));

  ObjectReferenceImpl* const new_object_reference =
      transaction_store_->CreateBoundObjectReference(name);
  CHECK(new_object_reference != nullptr);

  unordered_map<ObjectReferenceImpl*, vector<CommittedEvent*>> object_events;
  if (caller_object_reference != nullptr) {
    AddEventToMap(caller_object_reference,
                  new SubObjectCreationCommittedEvent(name,
                                                      new_object_reference),
                  &object_events);
  }
  AddEventToMap(new_object_reference,
                new ObjectCreationCommittedEvent(new_live_object),
                &object_events);
  AddTransactionEvents(object_events, caller_object_reference,
                       caller_live_object);

  pending_transaction_->AddNewObject(new_object_reference, new_live_object);

  if (!name.empty()) {
    // Send a GET_OBJECT message to remote peers so that the content of the
    // named object can eventually be synchronized with other peers.
    pending_transaction_->IsObjectKnown(new_object_reference);
  }

  return new_object_reference;
}

bool RecordingThread::CallMethod(
    ObjectReferenceImpl* caller_object_reference,
    const shared_ptr<LiveObject>& caller_live_object,
    ObjectReferenceImpl* callee_object_reference,
    const string& method_name,
    const vector<Value>& parameters,
    Value* return_value) {
  CHECK(callee_object_reference != nullptr);
  CHECK(!method_name.empty());
  CHECK(return_value != nullptr);

  if (Rewinding()) {
    return false;
  }

  // Record the METHOD_CALL event.
  {
    unordered_map<ObjectReferenceImpl*, shared_ptr<const LiveObject>>
        live_objects;
    unordered_map<ObjectReferenceImpl*, vector<CommittedEvent*>> object_events;

    if (caller_object_reference == callee_object_reference) {
      AddEventToMap(
          caller_object_reference,
          new SelfMethodCallCommittedEvent(
              unordered_set<ObjectReferenceImpl*>(), method_name, parameters),
          &object_events);
    } else{
      if (caller_object_reference != nullptr) {
        AddEventToMap(
            caller_object_reference,
            new SubMethodCallCommittedEvent(
                unordered_set<ObjectReferenceImpl*>(), callee_object_reference,
                method_name, parameters),
            &object_events);
      }

      AddEventToMap(callee_object_reference,
                    new MethodCallCommittedEvent(method_name, parameters),
                    &object_events);
    }

    AddTransactionEvents(object_events, caller_object_reference,
                         caller_live_object);
  }

  // Repeatedly try to call the method until either 1) the method succeeds, or
  // 2) a rewind action is requested.
  shared_ptr<LiveObject> callee_live_object;
  if (!CallMethodHelper(callee_object_reference, method_name, parameters,
                        return_value, &callee_live_object)) {
    // The current method is being rewound.
    return false;
  }

  // Record the METHOD_RETURN event.
  {
    unordered_map<ObjectReferenceImpl*, shared_ptr<const LiveObject>>
        live_objects;
    unordered_map<ObjectReferenceImpl*, vector<CommittedEvent*>> object_events;

    if (caller_object_reference == callee_object_reference) {
      AddEventToMap(
          caller_object_reference,
          new SelfMethodReturnCommittedEvent(
              unordered_set<ObjectReferenceImpl*>(), *return_value),
          &object_events);
    } else{
      if (caller_object_reference != nullptr) {
        AddEventToMap(caller_object_reference,
                      new SubMethodReturnCommittedEvent(*return_value),
                      &object_events);
      }

      AddEventToMap(
          callee_object_reference,
          new MethodReturnCommittedEvent(unordered_set<ObjectReferenceImpl*>(),
                                         *return_value),
          &object_events);
    }

    AddTransactionEvents(object_events, callee_object_reference,
                         callee_live_object);
  }

  return true;
}

bool RecordingThread::ObjectsAreIdentical(const ObjectReferenceImpl* a,
                                          const ObjectReferenceImpl* b) const {
  return transaction_store_->ObjectsAreIdentical(a, b);
}

bool RecordingThread::CallMethodHelper(
    ObjectReferenceImpl* callee_object_reference,
    const string& method_name,
    const vector<Value>& parameters,
    Value* return_value,
    shared_ptr<LiveObject>* callee_live_object) {
  CHECK(callee_live_object != nullptr);

  const TransactionId method_base_transaction_id =
      pending_transaction_->base_transaction_id();

  for (;;) {
    const shared_ptr<LiveObject> callee_live_object_temp =
        pending_transaction_->GetLiveObject(callee_object_reference);
    RecordingMethodContext method_context(this, callee_object_reference,
                                          callee_live_object_temp);

    callee_live_object_temp->InvokeMethod(&method_context,
                                          callee_object_reference, method_name,
                                          parameters, return_value);

    const TransactionStoreInternalInterface::ExecutionPhase execution_phase =
        transaction_store_->GetExecutionPhase(method_base_transaction_id);

    switch (execution_phase) {
      case TransactionStoreInternalInterface::NORMAL:
        *callee_live_object = callee_live_object_temp;
        return true;

      case TransactionStoreInternalInterface::REWIND:
        return false;

      case TransactionStoreInternalInterface::RESUME:
        // A rewind action was requested, but the rewind does not include the
        // current method call. Discard the old pending transaction and call
        // the child method again.
        pending_transaction_.reset(
            new PendingTransaction(
                transaction_store_, method_base_transaction_id,
                transaction_store_->GetCurrentSequencePoint()));

        // TODO(dss): Replace method calls with mocks until execution has
        // proceeded past the last unreverted transaction.
        break;

      default:
        LOG(FATAL) << "Invalid execution phase: "
                   << static_cast<int>(execution_phase);
    }
  }
}

void RecordingThread::AddTransactionEvent(
    ObjectReferenceImpl* event_object_reference,
    CommittedEvent* event,
    ObjectReferenceImpl* prev_object_reference,
    const shared_ptr<LiveObject>& prev_live_object) {
  unordered_map<ObjectReferenceImpl*, vector<CommittedEvent*>> object_events;
  object_events[event_object_reference].push_back(event);
  AddTransactionEvents(object_events, prev_object_reference, prev_live_object);
}

void RecordingThread::AddTransactionEvents(
    const unordered_map<ObjectReferenceImpl*, vector<CommittedEvent*>>&
        object_events,
    ObjectReferenceImpl* prev_object_reference,
    const shared_ptr<LiveObject>& prev_live_object) {
  CHECK(!object_events.empty());

  if (prev_object_reference != nullptr) {
    pending_transaction_->UpdateLiveObject(prev_object_reference,
                                           prev_live_object);
  }

  for (const auto& object_pair : object_events) {
    ObjectReferenceImpl* const object_reference = object_pair.first;
    const vector<CommittedEvent*>& events = object_pair.second;

    for (CommittedEvent* const event : events) {
      pending_transaction_->AddEvent(object_reference, event);
    }
  }

  if (pending_transaction_->transaction_level() == 0) {
    CommitTransaction();
  }
}

void RecordingThread::CommitTransaction() {
  VLOG(2) << "RecordingThread::CommitTransaction";

  TransactionId transaction_id;
  unordered_set<ObjectReferenceImpl*> transaction_new_objects;
  pending_transaction_->Commit(&transaction_id, &transaction_new_objects);

  pending_transaction_.reset(
      new PendingTransaction(transaction_store_, transaction_id,
                             transaction_store_->GetCurrentSequencePoint()));
}

bool RecordingThread::Rewinding() {
  return transaction_store_->GetExecutionPhase(
      pending_transaction_->base_transaction_id()) !=
      TransactionStoreInternalInterface::NORMAL;
}

}  // namespace engine
}  // namespace floating_temple
