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
#include <vector>

#include "base/logging.h"
#include "engine/live_object.h"
#include "engine/object_reference_impl.h"
#include "engine/pending_event.h"
#include "engine/pending_transaction.h"
#include "engine/proto/transaction_id.pb.h"
#include "engine/recording_method_context.h"
#include "engine/shared_object.h"
#include "engine/transaction_id_util.h"
#include "engine/transaction_store_internal_interface.h"
#include "include/c++/value.h"

using std::shared_ptr;
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

namespace floating_temple {

class ObjectReference;

namespace engine {

RecordingThread::RecordingThread(
    TransactionStoreInternalInterface* transaction_store)
    : transaction_store_(CHECK_NOTNULL(transaction_store)),
      pending_transaction_(new PendingTransaction(transaction_store,
                                                  MIN_TRANSACTION_ID)) {
}

RecordingThread::~RecordingThread() {
}

void RecordingThread::RunProgram(LocalObject* local_object,
                                 const string& method_name,
                                 Value* return_value,
                                 bool linger) {
  CHECK(return_value != nullptr);

  ObjectReferenceImpl* const object_reference = CreateObject(local_object, "");

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
  if (Rewinding()) {
    return false;
  }

  if (caller_object_reference != nullptr) {
    AddTransactionEvent(
        new BeginTransactionPendingEvent(caller_object_reference),
        caller_object_reference, caller_live_object);
  }

  pending_transaction_->IncrementTransactionLevel();

  return true;
}

bool RecordingThread::EndTransaction(
    ObjectReferenceImpl* caller_object_reference,
    const shared_ptr<LiveObject>& caller_live_object) {
  if (Rewinding()) {
    return false;
  }

  if (caller_object_reference != nullptr) {
    AddTransactionEvent(
        new EndTransactionPendingEvent(caller_object_reference),
        caller_object_reference, caller_live_object);
  }

  if (pending_transaction_->DecrementTransactionLevel()) {
    CommitTransaction();
  }

  return true;
}

ObjectReferenceImpl* RecordingThread::CreateObject(LocalObject* initial_version,
                                                   const string& name) {
  // Take ownership of *initial_version.
  shared_ptr<const LiveObject> new_live_object(new LiveObject(initial_version));

  ObjectReferenceImpl* object_reference = nullptr;

  if (name.empty()) {
    object_reference = transaction_store_->CreateUnboundObjectReference();

    NewObject new_object;
    new_object.live_object = new_live_object;
    new_object.object_is_named = false;

    CHECK(new_objects_.emplace(object_reference, new_object).second);
  } else {
    object_reference = transaction_store_->CreateBoundObjectReference(name);

    NewObject new_object;
    new_object.live_object = new_live_object;
    new_object.object_is_named = true;

    const auto insert_result = new_objects_.emplace(object_reference,
                                                    new_object);

    if (insert_result.second) {
      // The named object has not yet been created in this thread.

      // Check if the named object is already known to this peer. As a side
      // effect, send a GET_OBJECT message to remote peers so that the content
      // of the named object can eventually be synchronized with other peers.
      if (pending_transaction_->IsObjectKnown(object_reference)) {
        // The named object was already known to this peer. Remove the map entry
        // that was just added.
        const unordered_map<ObjectReferenceImpl*, NewObject>::iterator
            new_object_it = insert_result.first;
        CHECK(new_object_it != new_objects_.end());

        new_objects_.erase(new_object_it);
      }
    }
  }

  CHECK(object_reference != nullptr);
  return object_reference;
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
    unordered_set<ObjectReferenceImpl*> new_object_references;

    CheckIfObjectIsNew(caller_object_reference, &live_objects,
                       &new_object_references);
    CheckIfObjectIsNew(callee_object_reference, &live_objects,
                       &new_object_references);

    for (const Value& parameter : parameters) {
      CheckIfValueIsNew(parameter, &live_objects, &new_object_references);
    }

    PendingEvent* const pending_event = new MethodCallPendingEvent(
        live_objects, new_object_references, caller_object_reference,
        callee_object_reference, method_name, parameters);
    AddTransactionEvent(pending_event, caller_object_reference,
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
    unordered_set<ObjectReferenceImpl*> new_object_references;

    CheckIfValueIsNew(*return_value, &live_objects, &new_object_references);

    PendingEvent* const pending_event = new MethodReturnPendingEvent(
        live_objects, new_object_references, callee_object_reference,
        caller_object_reference, *return_value);
    AddTransactionEvent(pending_event, callee_object_reference,
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
            new PendingTransaction(transaction_store_,
                                   method_base_transaction_id));

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
    PendingEvent* event, ObjectReferenceImpl* current_object_reference,
    const shared_ptr<LiveObject>& current_live_object) {
  CHECK(event != nullptr);

  const bool first_event = pending_transaction_->IsEmpty();

  if (current_object_reference != nullptr) {
    pending_transaction_->UpdateLiveObject(current_object_reference,
                                           current_live_object);
  }
  pending_transaction_->AddEvent(event);

  if (pending_transaction_->transaction_level() == 0 &&
      !(first_event && event->prev_object_reference() == nullptr)) {
    CommitTransaction();
  }
}

void RecordingThread::CommitTransaction() {
  TransactionId transaction_id;
  unordered_set<ObjectReferenceImpl*> transaction_new_objects;
  pending_transaction_->Commit(&transaction_id, &transaction_new_objects);

  for (ObjectReferenceImpl* const object_reference : transaction_new_objects) {
    CHECK_EQ(new_objects_.erase(object_reference), 1u);
  }

  pending_transaction_.reset(new PendingTransaction(transaction_store_,
                                                    transaction_id));
}

void RecordingThread::CheckIfValueIsNew(
    const Value& value,
    unordered_map<ObjectReferenceImpl*, shared_ptr<const LiveObject>>*
        live_objects,
    unordered_set<ObjectReferenceImpl*>* new_object_references) {
  if (value.type() == Value::OBJECT_REFERENCE) {
    CheckIfObjectIsNew(
        static_cast<ObjectReferenceImpl*>(value.object_reference()),
        live_objects, new_object_references);
  }
}

void RecordingThread::CheckIfObjectIsNew(
    ObjectReferenceImpl* object_reference,
    unordered_map<ObjectReferenceImpl*, shared_ptr<const LiveObject>>*
        live_objects,
    unordered_set<ObjectReferenceImpl*>* new_object_references) {
  CHECK(live_objects != nullptr);
  CHECK(new_object_references != nullptr);

  if (object_reference != nullptr) {
    const unordered_map<ObjectReferenceImpl*, NewObject>::iterator it =
        new_objects_.find(object_reference);

    if (it != new_objects_.end()) {
      const NewObject& new_object = it->second;
      const shared_ptr<const LiveObject>& live_object = new_object.live_object;

      if (pending_transaction_->AddNewObject(object_reference, live_object)) {
        live_objects->emplace(object_reference, live_object);

        if (!new_object.object_is_named) {
          new_object_references->insert(object_reference);
        }
      }
    }
  }
}

bool RecordingThread::Rewinding() {
  return transaction_store_->GetExecutionPhase(
      pending_transaction_->base_transaction_id()) !=
      TransactionStoreInternalInterface::NORMAL;
}

}  // namespace engine
}  // namespace floating_temple
