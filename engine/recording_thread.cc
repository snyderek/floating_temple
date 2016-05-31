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

#include <pthread.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "base/cond_var.h"
#include "base/logging.h"
#include "base/mutex.h"
#include "base/mutex_lock.h"
#include "engine/live_object.h"
#include "engine/object_reference_impl.h"
#include "engine/pending_event.h"
#include "engine/proto/transaction_id.pb.h"
#include "engine/sequence_point.h"
#include "engine/shared_object.h"
#include "engine/transaction_id_util.h"
#include "engine/transaction_store_internal_interface.h"
#include "include/c++/value.h"

using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::unordered_set;
using std::vector;

namespace floating_temple {

class ObjectReference;

namespace engine {
namespace {

ObjectReferenceImpl* GetObjectReferenceForEvent(
    ObjectReferenceImpl* object_reference) {
  if (object_reference != nullptr) {
    return object_reference;
  }
  return nullptr;
}

}  // namespace

RecordingThread::RecordingThread(
    TransactionStoreInternalInterface* transaction_store)
    : transaction_store_(CHECK_NOTNULL(transaction_store)),
      transaction_level_(0),
      committing_transaction_(false),
      current_object_reference_(nullptr),
      current_transaction_id_(MIN_TRANSACTION_ID),
      rejected_transaction_id_(MIN_TRANSACTION_ID) {
}

RecordingThread::~RecordingThread() {
}

void RecordingThread::RunProgram(LocalObject* local_object,
                                 const string& method_name,
                                 Value* return_value,
                                 bool linger) {
  CHECK(return_value != nullptr);

  ObjectReference* const object_reference = CreateObject(local_object, "");

  for (;;) {
    Value return_value_temp;
    if (CallMethod(object_reference, method_name, vector<Value>(),
                   &return_value_temp)) {
      if (!linger) {
        *return_value = return_value_temp;
        return;
      }

      // TODO(dss): The following code is similar to code in
      // RecordingThread::CallMethodHelper. Consider merging the duplicate
      // functionality.

      MutexLock lock(&rejected_transaction_id_mu_);

      // The program completed successfully. Enter linger mode. This allows
      // execution of the current thread to be rewound if another peer rejects a
      // transaction from this peer.
      while (!Rewinding_Locked()) {
        // TODO(dss): Exit if the process receives SIGTERM.
        rewinding_cond_.WaitPatiently(&rejected_transaction_id_mu_);
      }

      // A rewind operation was requested. Wait for each blocking thread to call
      // RecordingThread::Resume before continuing.
      while (!blocking_threads_.empty()) {
        blocking_threads_empty_cond_.Wait(&rejected_transaction_id_mu_);
      }

      // Clear the rewind state.
      rejected_transaction_id_ = MIN_TRANSACTION_ID;
    }
  }
}

void RecordingThread::Rewind(const TransactionId& rejected_transaction_id) {
  MutexLock lock(&rejected_transaction_id_mu_);

  if (!Rewinding_Locked() ||
      rejected_transaction_id < rejected_transaction_id_) {
    rejected_transaction_id_ = rejected_transaction_id;
    rewinding_cond_.Broadcast();
  }

  CHECK(blocking_threads_.insert(pthread_self()).second);
}

void RecordingThread::Resume() {
  MutexLock lock(&rejected_transaction_id_mu_);

  CHECK_EQ(blocking_threads_.erase(pthread_self()), 1u);

  if (blocking_threads_.empty()) {
    blocking_threads_empty_cond_.Broadcast();
  }
}

bool RecordingThread::BeginTransaction() {
  CHECK_GE(transaction_level_, 0);

  if (Rewinding()) {
    return false;
  }

  if (current_object_reference_ != nullptr) {
    modified_objects_[current_object_reference_] = current_live_object_;
    AddTransactionEvent(
        new BeginTransactionPendingEvent(current_object_reference_));
  }

  ++transaction_level_;

  return true;
}

bool RecordingThread::EndTransaction() {
  CHECK_GT(transaction_level_, 0);

  if (Rewinding()) {
    return false;
  }

  if (current_object_reference_ != nullptr) {
    modified_objects_[current_object_reference_] = current_live_object_;
    AddTransactionEvent(
        new EndTransactionPendingEvent(current_object_reference_));
  }

  --transaction_level_;

  if (transaction_level_ == 0 && !events_.empty()) {
    CommitTransaction();
  }

  return true;
}

ObjectReference* RecordingThread::CreateObject(LocalObject* initial_version,
                                               const string& name) {
  // Take ownership of *initial_version.
  shared_ptr<const LiveObject> new_live_object(new LiveObject(initial_version));

  ObjectReferenceImpl* object_reference = nullptr;

  if (name.empty()) {
    if (transaction_store_->delay_object_binding()) {
      object_reference = transaction_store_->CreateUnboundObjectReference();

      NewObject new_object;
      new_object.live_object = new_live_object;
      new_object.object_is_named = false;

      CHECK(new_objects_.emplace(object_reference, new_object).second);
    } else {
      object_reference = transaction_store_->CreateBoundObjectReference("");

      AddTransactionEvent(
          new ObjectCreationPendingEvent(
              GetObjectReferenceForEvent(current_object_reference_),
              object_reference, new_live_object));
    }
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
      const shared_ptr<const LiveObject> existing_live_object =
          transaction_store_->GetLiveObjectAtSequencePoint(object_reference,
                                                           GetSequencePoint(),
                                                           false);

      if (existing_live_object.get() != nullptr) {
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

bool RecordingThread::CallMethod(ObjectReference* object_reference,
                                 const string& method_name,
                                 const vector<Value>& parameters,
                                 Value* return_value) {
  CHECK(object_reference != nullptr);
  CHECK(!method_name.empty());
  CHECK(return_value != nullptr);

  if (Rewinding()) {
    return false;
  }

  const vector<unique_ptr<PendingEvent>>::size_type event_count_save =
      events_.size();

  const TransactionId method_call_transaction_id = current_transaction_id_;

  ObjectReferenceImpl* const caller_object_reference =
      current_object_reference_;
  ObjectReferenceImpl* const callee_object_reference =
      static_cast<ObjectReferenceImpl*>(object_reference);

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

    if (caller_object_reference != nullptr) {
      modified_objects_[caller_object_reference] = current_live_object_;
    }

    AddTransactionEvent(
        new MethodCallPendingEvent(
            live_objects, new_object_references,
            GetObjectReferenceForEvent(caller_object_reference),
            GetObjectReferenceForEvent(callee_object_reference),
            method_name, parameters));
  }

  // Repeatedly try to call the method until either 1) the method succeeds, or
  // 2) a rewind action is requested.
  shared_ptr<LiveObject> callee_live_object;
  if (!CallMethodHelper(method_call_transaction_id, caller_object_reference,
                        callee_object_reference, method_name, parameters,
                        &callee_live_object, return_value)) {
    // The current method is being rewound.

    // If the METHOD_CALL event has not been committed yet, delete the event.
    // (If the event has been committed, then the transaction store is
    // responsible for deleting it.)
    if (current_transaction_id_ == method_call_transaction_id) {
      CHECK_GE(events_.size(), event_count_save);
      events_.resize(event_count_save);
    }

    // TODO(dss): Set transaction_level_ to the value it had when
    // RecordingThread::CallMethod was called.

    return false;
  }

  // Record the METHOD_RETURN event.
  {
    unordered_map<ObjectReferenceImpl*, shared_ptr<const LiveObject>>
        live_objects;
    unordered_set<ObjectReferenceImpl*> new_object_references;

    CheckIfValueIsNew(*return_value, &live_objects, &new_object_references);
    modified_objects_[callee_object_reference] = callee_live_object;

    AddTransactionEvent(
        new MethodReturnPendingEvent(
            live_objects, new_object_references,
            GetObjectReferenceForEvent(callee_object_reference),
            GetObjectReferenceForEvent(caller_object_reference),
            *return_value));
  }

  return true;
}

bool RecordingThread::ObjectsAreIdentical(const ObjectReference* a,
                                          const ObjectReference* b) const {
  return transaction_store_->ObjectsAreIdentical(
      static_cast<const ObjectReferenceImpl*>(a),
      static_cast<const ObjectReferenceImpl*>(b));
}

bool RecordingThread::CallMethodHelper(
    const TransactionId& method_call_transaction_id,
    ObjectReferenceImpl* caller_object_reference,
    ObjectReferenceImpl* callee_object_reference,
    const string& method_name,
    const vector<Value>& parameters,
    shared_ptr<LiveObject>* callee_live_object,
    Value* return_value) {
  CHECK(callee_live_object != nullptr);

  for (;;) {
    // TODO(dss): If the caller object has been modified by another peer since
    // the method was called, rewind.

    const shared_ptr<LiveObject> caller_live_object = current_live_object_;
    const shared_ptr<LiveObject> callee_live_object_temp = GetLiveObject(
        callee_object_reference);

    current_object_reference_ = callee_object_reference;
    current_live_object_ = callee_live_object_temp;

    callee_live_object_temp->InvokeMethod(this, callee_object_reference,
                                          method_name, parameters,
                                          return_value);

    current_live_object_ = caller_live_object;
    current_object_reference_ = caller_object_reference;

    {
      MutexLock lock(&rejected_transaction_id_mu_);

      if (!Rewinding_Locked()) {
        CHECK_EQ(blocking_threads_.size(), 0u);
        *callee_live_object = callee_live_object_temp;
        return true;
      }

      if (!WaitForBlockingThreads_Locked(method_call_transaction_id)) {
        return false;
      }

      // A rewind action was requested, but the rewind does not include the
      // current method call. Clear the rejected_transaction_id_ member variable
      // and call the method again.
      rejected_transaction_id_ = MIN_TRANSACTION_ID;
    }
  }

  return false;
}

// Waits for blocking_threads_ to be empty, which indicates it's safe to resume
// execution. Returns true if blocking_threads_ is empty. Returns false if
// another thread initiates a rewind operation during the wait.
bool RecordingThread::WaitForBlockingThreads_Locked(
    const TransactionId& method_call_transaction_id) const {
  for (;;) {
    if (rejected_transaction_id_ <= method_call_transaction_id) {
      return false;
    }

    if (blocking_threads_.empty()) {
      return true;
    }

    blocking_threads_empty_cond_.Wait(&rejected_transaction_id_mu_);
  }
}

shared_ptr<LiveObject> RecordingThread::GetLiveObject(
    ObjectReferenceImpl* object_reference) {
  CHECK(object_reference != nullptr);
  // If the object was in new_objects_, it already should have been moved to
  // modified_objects_ by RecordingThread::CheckIfObjectIsNew.
  CHECK(new_objects_.find(object_reference) == new_objects_.end());

  shared_ptr<LiveObject>& live_object = modified_objects_[object_reference];

  if (live_object.get() == nullptr) {
    const shared_ptr<const LiveObject> existing_live_object =
        transaction_store_->GetLiveObjectAtSequencePoint(object_reference,
                                                         GetSequencePoint(),
                                                         true);
    live_object = existing_live_object->Clone();
  }

  return live_object;
}

const SequencePoint* RecordingThread::GetSequencePoint() {
  if (sequence_point_.get() == nullptr) {
    sequence_point_.reset(transaction_store_->GetCurrentSequencePoint());
  }

  return sequence_point_.get();
}

void RecordingThread::AddTransactionEvent(PendingEvent* event) {
  CHECK_GE(transaction_level_, 0);
  CHECK(event != nullptr);

  const bool first_event = events_.empty();

  events_.emplace_back(event);

  if (transaction_level_ == 0 &&
      !(first_event && event->prev_object_reference() == nullptr)) {
    CommitTransaction();
  }
}

void RecordingThread::CommitTransaction() {
  CHECK(!events_.empty());

  // Prevent infinite recursion.
  if (committing_transaction_) {
    return;
  }

  committing_transaction_ = true;

  while (!events_.empty()) {
    vector<unique_ptr<PendingEvent>> events_to_commit;
    unordered_map<ObjectReferenceImpl*, shared_ptr<LiveObject>>
        modified_objects_to_commit;
    events_to_commit.swap(events_);
    modified_objects_to_commit.swap(modified_objects_);

    transaction_store_->CreateTransaction(events_to_commit,
                                          &current_transaction_id_,
                                          modified_objects_to_commit,
                                          GetSequencePoint());

    // TODO(dss): [Optimization] Set sequence_point_ to NULL here and only call
    // transaction_store_->GetCurrentSequencePoint when the sequence point is
    // actually needed.
    sequence_point_.reset(transaction_store_->GetCurrentSequencePoint());
  }

  CHECK(committing_transaction_);
  committing_transaction_ = false;
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

      live_objects->emplace(object_reference, live_object);

      if (!new_object.object_is_named) {
        new_object_references->insert(object_reference);
      }

      // Make the object available to other methods in the same transaction.
      // Subsequent transactions will be able to fetch the object from the
      // transaction store.
      CHECK(modified_objects_.emplace(object_reference,
                                      live_object->Clone()).second);

      new_objects_.erase(it);
    }
  }
}

bool RecordingThread::Rewinding() const {
  MutexLock lock(&rejected_transaction_id_mu_);
  return Rewinding_Locked();
}

bool RecordingThread::Rewinding_Locked() const {
  return rejected_transaction_id_ > MIN_TRANSACTION_ID;
}

}  // namespace engine
}  // namespace floating_temple
