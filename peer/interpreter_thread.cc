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

#include "peer/interpreter_thread.h"

#include <pthread.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/cond_var.h"
#include "base/linked_ptr.h"
#include "base/logging.h"
#include "base/mutex.h"
#include "base/mutex_lock.h"
#include "include/c++/value.h"
#include "peer/const_live_object_ptr.h"
#include "peer/live_object.h"
#include "peer/live_object_ptr.h"
#include "peer/peer_object_impl.h"
#include "peer/pending_event.h"
#include "peer/proto/transaction_id.pb.h"
#include "peer/sequence_point.h"
#include "peer/transaction_id_util.h"
#include "peer/transaction_store_internal_interface.h"

using std::pair;
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

namespace floating_temple {

class PeerObject;

namespace peer {

InterpreterThread::InterpreterThread(
    TransactionStoreInternalInterface* transaction_store)
    : transaction_store_(CHECK_NOTNULL(transaction_store)),
      transaction_level_(0),
      committing_transaction_(false),
      current_peer_object_(nullptr) {
  GetMinTransactionId(&current_transaction_id_);
  GetMinTransactionId(&rejected_transaction_id_);
}

InterpreterThread::~InterpreterThread() {
}

void InterpreterThread::RunProgram(VersionedLocalObject* local_object,
                                   const string& method_name,
                                   Value* return_value,
                                   bool linger) {
  CHECK(return_value != nullptr);

  PeerObject* const peer_object = CreatePeerObject(local_object, "", false);

  for (;;) {
    Value return_value_temp;
    if (CallMethod(peer_object, method_name, vector<Value>(),
                   &return_value_temp)) {
      if (!linger) {
        *return_value = return_value_temp;
        return;
      }

      // TODO(dss): The following code is similar to code in
      // InterpreterThread::CallMethodHelper. Consider merging the duplicate
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
      // InterpreterThread::Resume before continuing.
      while (!blocking_threads_.empty()) {
        blocking_threads_empty_cond_.Wait(&rejected_transaction_id_mu_);
      }

      // Clear the rewind state.
      GetMinTransactionId(&rejected_transaction_id_);
    }
  }
}

void InterpreterThread::Rewind(const TransactionId& rejected_transaction_id) {
  MutexLock lock(&rejected_transaction_id_mu_);

  if (!Rewinding_Locked() ||
      CompareTransactionIds(rejected_transaction_id,
                            rejected_transaction_id_) < 0) {
    rejected_transaction_id_.CopyFrom(rejected_transaction_id);
    rewinding_cond_.Broadcast();
  }

  CHECK(blocking_threads_.insert(pthread_self()).second);
}

void InterpreterThread::Resume() {
  MutexLock lock(&rejected_transaction_id_mu_);

  CHECK_EQ(blocking_threads_.erase(pthread_self()), 1u);

  if (blocking_threads_.empty()) {
    blocking_threads_empty_cond_.Broadcast();
  }
}

bool InterpreterThread::BeginTransaction() {
  CHECK_GE(transaction_level_, 0);

  if (Rewinding()) {
    return false;
  }

  if (current_peer_object_ != nullptr) {
    modified_objects_[current_peer_object_] = current_live_object_;
    AddTransactionEvent(new BeginTransactionPendingEvent(current_peer_object_));
  }

  ++transaction_level_;

  return true;
}

bool InterpreterThread::EndTransaction() {
  CHECK_GT(transaction_level_, 0);

  if (Rewinding()) {
    return false;
  }

  if (current_peer_object_ != nullptr) {
    modified_objects_[current_peer_object_] = current_live_object_;
    AddTransactionEvent(new EndTransactionPendingEvent(current_peer_object_));
  }

  --transaction_level_;

  if (transaction_level_ == 0 && !events_.empty()) {
    CommitTransaction();
  }

  return true;
}

PeerObject* InterpreterThread::CreatePeerObject(
    VersionedLocalObject* initial_version, const string& name, bool versioned) {
  // Take ownership of *initial_version.
  ConstLiveObjectPtr new_live_object(new LiveObject(initial_version));

  PeerObjectImpl* peer_object = nullptr;

  if (name.empty()) {
    if (transaction_store_->delay_object_binding()) {
      peer_object = transaction_store_->CreateUnboundPeerObject(versioned);

      NewObject new_object;
      new_object.live_object = new_live_object;
      new_object.object_is_named = false;

      CHECK(new_objects_.emplace(peer_object, new_object).second);
    } else {
      peer_object = transaction_store_->CreateBoundPeerObject("", versioned);

      AddTransactionEvent(new ObjectCreationPendingEvent(current_peer_object_,
                                                         peer_object,
                                                         new_live_object));
    }
  } else {
    peer_object = transaction_store_->CreateBoundPeerObject(name, versioned);

    NewObject new_object;
    new_object.live_object = new_live_object;
    new_object.object_is_named = true;

    const pair<unordered_map<PeerObjectImpl*, NewObject>::iterator, bool>
        insert_result = new_objects_.emplace(peer_object, new_object);

    if (insert_result.second) {
      // The named object has not yet been created in this thread.

      // Check if the named object is already known to this peer. As a side
      // effect, send a GET_OBJECT message to remote peers so that the content
      // of the named object can eventually be synchronized with other peers.
      const ConstLiveObjectPtr existing_live_object =
          transaction_store_->GetLiveObjectAtSequencePoint(peer_object,
                                                           GetSequencePoint(),
                                                           false);

      if (existing_live_object.get() != nullptr) {
        // The named object was already known to this peer. Remove the map entry
        // that was just added.
        const unordered_map<PeerObjectImpl*, NewObject>::iterator
            new_object_it = insert_result.first;
        CHECK(new_object_it != new_objects_.end());

        new_objects_.erase(new_object_it);
      }
    }
  }

  CHECK(peer_object != nullptr);
  return peer_object;
}

bool InterpreterThread::CallMethod(PeerObject* peer_object,
                                   const string& method_name,
                                   const vector<Value>& parameters,
                                   Value* return_value) {
  CHECK(peer_object != nullptr);
  CHECK(!method_name.empty());
  CHECK(return_value != nullptr);

  if (Rewinding()) {
    return false;
  }

  const vector<linked_ptr<PendingEvent>>::size_type event_count_save =
      events_.size();

  TransactionId method_call_transaction_id;
  method_call_transaction_id.CopyFrom(current_transaction_id_);

  PeerObjectImpl* const caller_peer_object = current_peer_object_;
  PeerObjectImpl* const callee_peer_object = static_cast<PeerObjectImpl*>(
      peer_object);

  // Record the METHOD_CALL event.
  {
    unordered_map<PeerObjectImpl*, ConstLiveObjectPtr> live_objects;
    unordered_set<PeerObjectImpl*> new_peer_objects;

    CheckIfPeerObjectIsNew(caller_peer_object, &live_objects,
                           &new_peer_objects);
    CheckIfPeerObjectIsNew(callee_peer_object, &live_objects,
                           &new_peer_objects);

    for (const Value& parameter : parameters) {
      CheckIfValueIsNew(parameter, &live_objects, &new_peer_objects);
    }

    if (caller_peer_object != nullptr) {
      modified_objects_[caller_peer_object] = current_live_object_;
    }

    AddTransactionEvent(new MethodCallPendingEvent(live_objects,
                                                   new_peer_objects,
                                                   caller_peer_object,
                                                   callee_peer_object,
                                                   method_name, parameters));
  }

  // Repeatedly try to call the method until either 1) the method succeeds, or
  // 2) a rewind action is requested.
  LiveObjectPtr callee_live_object;
  if (!CallMethodHelper(method_call_transaction_id, caller_peer_object,
                        callee_peer_object, method_name, parameters,
                        &callee_live_object, return_value)) {
    // The current method is being rewound.

    // If the METHOD_CALL event has not been committed yet, delete the event.
    // (If the event has been committed, then the transaction store is
    // responsible for deleting it.)
    if (CompareTransactionIds(current_transaction_id_,
                              method_call_transaction_id) == 0) {
      CHECK_GT(events_.size(), event_count_save);
      events_.resize(event_count_save);
    }

    return false;
  }

  // Record the METHOD_RETURN event.
  {
    unordered_map<PeerObjectImpl*, ConstLiveObjectPtr> live_objects;
    unordered_set<PeerObjectImpl*> new_peer_objects;

    CheckIfValueIsNew(*return_value, &live_objects, &new_peer_objects);

    modified_objects_[callee_peer_object] = callee_live_object;

    AddTransactionEvent(new MethodReturnPendingEvent(live_objects,
                                                     new_peer_objects,
                                                     callee_peer_object,
                                                     caller_peer_object,
                                                     *return_value));
  }

  return true;
}

bool InterpreterThread::ObjectsAreEquivalent(const PeerObject* a,
                                             const PeerObject* b) const {
  return transaction_store_->ObjectsAreEquivalent(
      static_cast<const PeerObjectImpl*>(a),
      static_cast<const PeerObjectImpl*>(b));
}

bool InterpreterThread::CallMethodHelper(
    const TransactionId& method_call_transaction_id,
    PeerObjectImpl* caller_peer_object,
    PeerObjectImpl* callee_peer_object,
    const string& method_name,
    const vector<Value>& parameters,
    LiveObjectPtr* callee_live_object,
    Value* return_value) {
  CHECK(callee_live_object != nullptr);

  for (;;) {
    // TODO(dss): If the caller object has been modified by another peer since
    // the method was called, rewind.

    const LiveObjectPtr caller_live_object = current_live_object_;
    const LiveObjectPtr callee_live_object_temp = GetLiveObject(
        callee_peer_object);

    current_peer_object_ = callee_peer_object;
    current_live_object_ = callee_live_object_temp;

    callee_live_object_temp->InvokeMethod(this, callee_peer_object, method_name,
                                          parameters, return_value);

    current_live_object_ = caller_live_object;
    current_peer_object_ = caller_peer_object;

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
      GetMinTransactionId(&rejected_transaction_id_);
    }
  }

  return false;
}

// Waits for blocking_threads_ to be empty, which indicates it's safe to resume
// execution. Returns true if blocking_threads_ is empty. Returns false if
// another thread initiates a rewind operation during the wait.
bool InterpreterThread::WaitForBlockingThreads_Locked(
    const TransactionId& method_call_transaction_id) const {
  for (;;) {
    if (CompareTransactionIds(rejected_transaction_id_,
                              method_call_transaction_id) <= 0) {
      return false;
    }

    if (blocking_threads_.empty()) {
      return true;
    }

    blocking_threads_empty_cond_.Wait(&rejected_transaction_id_mu_);
  }
}

LiveObjectPtr InterpreterThread::GetLiveObject(PeerObjectImpl* peer_object) {
  CHECK(peer_object != nullptr);
  // If the peer object was in new_objects_, it already should have been moved
  // to modified_objects_ by InterpreterThread::CheckIfPeerObjectIsNew.
  CHECK(new_objects_.find(peer_object) == new_objects_.end());

  LiveObjectPtr& live_object = modified_objects_[peer_object];

  if (live_object.get() == nullptr) {
    const ConstLiveObjectPtr existing_live_object =
        transaction_store_->GetLiveObjectAtSequencePoint(peer_object,
                                                         GetSequencePoint(),
                                                         true);
    live_object = existing_live_object->Clone();
  }

  return live_object;
}

const SequencePoint* InterpreterThread::GetSequencePoint() {
  if (sequence_point_.get() == nullptr) {
    sequence_point_.reset(transaction_store_->GetCurrentSequencePoint());
  }

  return sequence_point_.get();
}

void InterpreterThread::AddTransactionEvent(PendingEvent* event) {
  CHECK_GE(transaction_level_, 0);
  CHECK(event != nullptr);

  const bool first_event = events_.empty();

  events_.emplace_back(event);

  if (transaction_level_ == 0 &&
      !(first_event && event->prev_peer_object() == nullptr)) {
    CommitTransaction();
  }
}

void InterpreterThread::CommitTransaction() {
  CHECK(!events_.empty());

  // Prevent infinite recursion.
  if (committing_transaction_) {
    return;
  }

  committing_transaction_ = true;

  while (!events_.empty()) {
    vector<linked_ptr<PendingEvent>> events_to_commit;
    unordered_map<PeerObjectImpl*, LiveObjectPtr> modified_objects_to_commit;
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

void InterpreterThread::CheckIfValueIsNew(
    const Value& value,
    unordered_map<PeerObjectImpl*, ConstLiveObjectPtr>* live_objects,
    unordered_set<PeerObjectImpl*>* new_peer_objects) {
  if (value.type() == Value::PEER_OBJECT) {
    CheckIfPeerObjectIsNew(static_cast<PeerObjectImpl*>(value.peer_object()),
                           live_objects, new_peer_objects);
  }
}

void InterpreterThread::CheckIfPeerObjectIsNew(
    PeerObjectImpl* peer_object,
    unordered_map<PeerObjectImpl*, ConstLiveObjectPtr>* live_objects,
    unordered_set<PeerObjectImpl*>* new_peer_objects) {
  CHECK(live_objects != nullptr);
  CHECK(new_peer_objects != nullptr);

  if (peer_object != nullptr) {
    const unordered_map<PeerObjectImpl*, NewObject>::iterator it =
        new_objects_.find(peer_object);

    if (it != new_objects_.end()) {
      const NewObject& new_object = it->second;
      const ConstLiveObjectPtr& live_object = new_object.live_object;

      live_objects->emplace(peer_object, live_object);

      if (!new_object.object_is_named) {
        new_peer_objects->insert(peer_object);
      }

      // Make the object available to other methods in the same transaction.
      // Subsequent transactions will be able to fetch the object from the
      // transaction store.
      CHECK(modified_objects_.emplace(peer_object,
                                      live_object->Clone()).second);

      new_objects_.erase(it);
    }
  }
}

bool InterpreterThread::Rewinding() const {
  MutexLock lock(&rejected_transaction_id_mu_);
  return Rewinding_Locked();
}

bool InterpreterThread::Rewinding_Locked() const {
  return IsValidTransactionId(rejected_transaction_id_);
}

}  // namespace peer
}  // namespace floating_temple
