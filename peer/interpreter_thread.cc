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

#include <cstddef>
#include <string>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include <utility>
#include <vector>

#include "base/cond_var.h"
#include "base/linked_ptr.h"
#include "base/logging.h"
#include "base/mutex.h"
#include "base/mutex_lock.h"
#include "base/scoped_ptr.h"
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

using std::make_pair;
using std::pair;
using std::string;
using std::tr1::unordered_map;
using std::tr1::unordered_set;
using std::vector;

namespace floating_temple {
namespace peer {

InterpreterThread::InterpreterThread(
    TransactionStoreInternalInterface* transaction_store)
    : transaction_store_(CHECK_NOTNULL(transaction_store)),
      transaction_level_(0),
      current_peer_object_(NULL) {
  GetMinTransactionId(&current_transaction_id_);
  GetMinTransactionId(&rejected_transaction_id_);
}

InterpreterThread::~InterpreterThread() {
}

void InterpreterThread::Rewind(const TransactionId& rejected_transaction_id) {
  MutexLock lock(&rejected_transaction_id_mu_);

  if (!Rewinding_Locked() ||
      CompareTransactionIds(rejected_transaction_id,
                            rejected_transaction_id_) < 0) {
    rejected_transaction_id_.CopyFrom(rejected_transaction_id);
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

  if (current_peer_object_ != NULL) {
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

  if (current_peer_object_ != NULL) {
    modified_objects_[current_peer_object_] = current_live_object_;
    AddTransactionEvent(new EndTransactionPendingEvent(current_peer_object_));
  }

  --transaction_level_;

  if (transaction_level_ == 0 && !events_.empty()) {
    CommitTransaction();
  }

  return true;
}

PeerObject* InterpreterThread::CreatePeerObject(LocalObject* initial_version) {
  // Take ownership of *initial_version.
  ConstLiveObjectPtr new_live_object(new LiveObject(initial_version));

  PeerObjectImpl* const peer_object =
      transaction_store_->CreateUnboundPeerObject();

  if (transaction_store_->delay_object_binding()) {
    pair<PeerObjectImpl*, NewObject> new_map_value;
    new_map_value.first = peer_object;
    NewObject* const new_object = &new_map_value.second;
    new_object->live_object = new_live_object;
    new_object->object_is_named = false;

    CHECK(new_objects_.insert(new_map_value).second);
  } else {
    AddTransactionEvent(new ObjectCreationPendingEvent(current_peer_object_,
                                                       peer_object,
                                                       new_live_object));
  }

  return peer_object;
}

PeerObject* InterpreterThread::GetOrCreateNamedObject(
    const string& name, LocalObject* initial_version) {
  // Take ownership of *initial_version.
  ConstLiveObjectPtr new_live_object(new LiveObject(initial_version));

  // The peer object pointer is strictly a function of the object name.
  PeerObjectImpl* const peer_object =
      transaction_store_->GetOrCreateNamedObject(name);

  pair<PeerObjectImpl*, NewObject> new_object_map_value;
  new_object_map_value.first = peer_object;
  NewObject* const new_object = &new_object_map_value.second;
  new_object->live_object = new_live_object;
  new_object->object_is_named = true;

  const pair<unordered_map<PeerObjectImpl*, NewObject>::iterator, bool>
      insert_result = new_objects_.insert(new_object_map_value);

  if (insert_result.second) {
    // The named object has not yet been created in this thread.

    // Check if the named object is already known to this peer. As a side
    // effect, send a GET_OBJECT message to remote peers so that the content of
    // the named object can eventually be synchronized with other peers.
    const ConstLiveObjectPtr existing_live_object =
        transaction_store_->GetLiveObjectAtSequencePoint(peer_object,
                                                         GetSequencePoint(),
                                                         false);

    if (existing_live_object.get() != NULL) {
      // The named object was already known to this peer. Remove the map entry
      // that was just added.
      const unordered_map<PeerObjectImpl*, NewObject>::iterator new_object_it =
          insert_result.first;
      CHECK(new_object_it != new_objects_.end());

      new_objects_.erase(new_object_it);
    }
  }

  return peer_object;
}

bool InterpreterThread::CallMethod(PeerObject* peer_object,
                                   const string& method_name,
                                   const vector<Value>& parameters,
                                   Value* return_value) {
  CHECK(peer_object != NULL);
  CHECK(!method_name.empty());
  CHECK(return_value != NULL);

  if (Rewinding()) {
    return false;
  }

  const vector<linked_ptr<PendingEvent> >::size_type event_count_save =
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

    if (caller_peer_object != NULL) {
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
  CHECK(callee_live_object != NULL);

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
  CHECK(peer_object != NULL);
  // If the peer object was in new_objects_, it already should have been moved
  // to modified_objects_ by InterpreterThread::CheckIfPeerObjectIsNew.
  CHECK(new_objects_.find(peer_object) == new_objects_.end());

  LiveObjectPtr& live_object = modified_objects_[peer_object];

  if (live_object.get() == NULL) {
    const ConstLiveObjectPtr existing_live_object =
        transaction_store_->GetLiveObjectAtSequencePoint(peer_object,
                                                         GetSequencePoint(),
                                                         true);
    live_object = existing_live_object->Clone();
  }

  return live_object;
}

const SequencePoint* InterpreterThread::GetSequencePoint() {
  if (sequence_point_.get() == NULL) {
    sequence_point_.reset(transaction_store_->GetCurrentSequencePoint());
  }

  return sequence_point_.get();
}

void InterpreterThread::AddTransactionEvent(PendingEvent* event) {
  CHECK_GE(transaction_level_, 0);
  CHECK(event != NULL);

  const bool first_event = events_.empty();

  events_.push_back(make_linked_ptr(event));

  if (transaction_level_ == 0 &&
      !(first_event && event->prev_peer_object() == NULL)) {
    CommitTransaction();
  }
}

void InterpreterThread::CommitTransaction() {
  CHECK(!events_.empty());

  transaction_store_->CreateTransaction(events_, &current_transaction_id_,
                                        modified_objects_, GetSequencePoint());

  events_.clear();
  modified_objects_.clear();
  // TODO(dss): [Optimization] Set sequence_point_ to NULL here and only call
  // transaction_store_->GetCurrentSequencePoint when the sequence point is
  // actually needed.
  sequence_point_.reset(transaction_store_->GetCurrentSequencePoint());
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
  CHECK(live_objects != NULL);
  CHECK(new_peer_objects != NULL);

  if (peer_object != NULL) {
    const unordered_map<PeerObjectImpl*, NewObject>::iterator it =
        new_objects_.find(peer_object);

    if (it != new_objects_.end()) {
      const NewObject& new_object = it->second;
      const ConstLiveObjectPtr& live_object = new_object.live_object;

      live_objects->insert(make_pair(peer_object, live_object));

      if (!new_object.object_is_named) {
        new_peer_objects->insert(peer_object);
      }

      // Make the object available to other methods in the same transaction.
      // Subsequent transactions will be able to fetch the object from the
      // transaction store.
      CHECK(modified_objects_.insert(make_pair(peer_object,
                                               live_object->Clone())).second);

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
