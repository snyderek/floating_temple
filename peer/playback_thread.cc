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

#include "peer/playback_thread.h"

#include <pthread.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <gflags/gflags.h>

#include "base/escape.h"
#include "base/linked_ptr.h"
#include "base/logging.h"
#include "base/string_printf.h"
#include "include/c++/local_object.h"
#include "include/c++/unversioned_local_object.h"
#include "include/c++/value.h"
#include "include/c++/versioned_local_object.h"
#include "peer/committed_event.h"
#include "peer/committed_value.h"
#include "peer/convert_value.h"
#include "peer/event_queue.h"
#include "peer/live_object.h"
#include "peer/peer_object_impl.h"
#include "peer/shared_object.h"
#include "peer/transaction_store_internal_interface.h"
#include "util/bool_variable.h"
#include "util/state_variable.h"
#include "util/state_variable_internal_interface.h"

using std::pair;
using std::shared_ptr;
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

DEFINE_bool(treat_conflicts_as_fatal_for_debugging, false,
            "Log a FATAL error if a conflict occurs while replaying "
            "transactions on an object");

namespace floating_temple {
namespace peer {

PlaybackThread::PlaybackThread()
    : transaction_store_(nullptr),
      shared_object_(nullptr),
      new_peer_objects_(nullptr),
      conflict_detected_(false),
      state_(NOT_STARTED) {
  state_.AddStateTransition(NOT_STARTED, STARTING);
  state_.AddStateTransition(STARTING, RUNNING);
  state_.AddStateTransition(RUNNING, PAUSED);
  state_.AddStateTransition(PAUSED, RUNNING);
  state_.AddStateTransition(PAUSED, STOPPING);
  state_.AddStateTransition(STOPPING, STOPPED);
}

PlaybackThread::~PlaybackThread() {
  state_.CheckState(NOT_STARTED | STOPPED);
}

shared_ptr<const LiveObject> PlaybackThread::live_object() const {
  return live_object_;
}

void PlaybackThread::Start(
    TransactionStoreInternalInterface* transaction_store,
    SharedObject* shared_object,
    const shared_ptr<LiveObject>& live_object,
    unordered_map<SharedObject*, PeerObjectImpl*>* new_peer_objects) {
  CHECK(transaction_store != nullptr);
  CHECK(shared_object != nullptr);
  CHECK(new_peer_objects != nullptr);

  state_.ChangeState(STARTING);

  transaction_store_ = transaction_store;
  shared_object_ = shared_object;
  live_object_ = live_object;
  new_peer_objects_ = new_peer_objects;

  // TODO(dss): There may be a performance cost associated with creating and
  // destroying threads. Consider recycling the threads that are used by the
  // PlaybackThread class.
  CHECK_PTHREAD_ERR(pthread_create(&replay_thread_, nullptr,
                                   &PlaybackThread::ReplayThreadMain, this));

  state_.ChangeState(RUNNING);
}

void PlaybackThread::Stop() {
  state_.Mutate(&PlaybackThread::ChangePausedToRunning);
  event_queue_.SetEndOfSequence();
  state_.Mutate(&PlaybackThread::WaitForPausedAndChangeToStopping);

  void* thread_return_value = nullptr;
  CHECK_PTHREAD_ERR(pthread_join(replay_thread_, &thread_return_value));

  state_.ChangeState(STOPPED);
}

void PlaybackThread::QueueEvent(const CommittedEvent* event) {
  state_.Mutate(&PlaybackThread::ChangePausedToRunning);
  event_queue_.QueueEvent(event);
}

void PlaybackThread::FlushEvents() {
  state_.Mutate(&PlaybackThread::ChangePausedToRunning);
  event_queue_.SetEndOfSequence();
  state_.WaitForState(PAUSED);
}

void PlaybackThread::ReplayEvents() {
  state_.WaitForNotState(NOT_STARTED | STARTING);

  while (!conflict_detected_.Get() &&
         CheckNextEventType(CommittedEvent::METHOD_CALL)) {
    DoMethodCall();
  }

  // If a conflict has been detected, dequeue any remaining events and discard
  // them.
  while (HasNextEvent()) {
    GetNextEvent();
  }

  state_.Mutate(&PlaybackThread::ChangeRunningToPaused);
  unbound_peer_objects_.clear();
}

void PlaybackThread::DoMethodCall() {
  CHECK(live_object_.get() != nullptr);
  CHECK(!conflict_detected_.Get());

  if (!CheckNextEventType(CommittedEvent::METHOD_CALL)) {
    return;
  }

  string method_name;
  vector<Value> parameters;
  {
    SharedObject* caller = nullptr;
    const string* method_name_temp = nullptr;
    const vector<CommittedValue>* committed_parameters = nullptr;

    GetNextEvent()->GetMethodCall(&caller, &method_name_temp,
                                  &committed_parameters);

    method_name = *method_name_temp;
    VLOG(2) << "method_name == \"" << CEscape(method_name) << "\"";

    parameters.resize(committed_parameters->size());
    for (vector<CommittedValue>::size_type i = 0;
         i < committed_parameters->size(); ++i) {
      ConvertCommittedValueToValue((*committed_parameters)[i], &parameters[i]);
    }
  }

  if (!HasNextEvent()) {
    return;
  }

  PeerObjectImpl* const peer_object = shared_object_->GetOrCreatePeerObject(
      true);

  Value return_value;
  live_object_->InvokeMethod(this, peer_object, method_name, parameters,
                             &return_value);

  if (conflict_detected_.Get() ||
      !CheckNextEventType(CommittedEvent::METHOD_RETURN)) {
    return;
  }

  {
    SharedObject* caller = nullptr;
    const CommittedValue* expected_return_value = nullptr;

    const CommittedEvent* const event = GetNextEvent();
    event->GetMethodReturn(&caller, &expected_return_value);

    if (caller == shared_object_) {
      SetConflictDetected("Caller is the same as callee, but a self method "
                          "return was not expected.");
      return;
    }

    if (!ValueMatches(*expected_return_value, return_value,
                      event->new_shared_objects())) {
      SetConflictDetected("Return value doesn't match expected return value.");
      return;
    }
  }
}

void PlaybackThread::DoSelfMethodCall(PeerObjectImpl* peer_object,
                                      const string& method_name,
                                      const vector<Value>& parameters,
                                      Value* return_value) {
  CHECK(live_object_.get() != nullptr);
  CHECK(!conflict_detected_.Get());
  CHECK(return_value != nullptr);

  if (!CheckNextEventType(CommittedEvent::SELF_METHOD_CALL)) {
    return;
  }

  {
    const string* expected_method_name = nullptr;
    const vector<CommittedValue>* expected_parameters = nullptr;

    const CommittedEvent* const event = GetNextEvent();
    event->GetSelfMethodCall(&expected_method_name, &expected_parameters);

    if (!MethodCallMatches(shared_object_, *expected_method_name,
                           *expected_parameters, peer_object, method_name,
                           parameters, event->new_shared_objects())) {
      SetConflictDetected("Self method call doesn't match expected method "
                          "call.");
      return;
    }
  }

  if (!HasNextEvent()) {
    return;
  }

  live_object_->InvokeMethod(this, peer_object, method_name, parameters,
                             return_value);

  if (conflict_detected_.Get() ||
      !CheckNextEventType(CommittedEvent::SELF_METHOD_RETURN)) {
    return;
  }

  {
    const CommittedValue* expected_return_value = nullptr;

    const CommittedEvent* const event = GetNextEvent();
    event->GetSelfMethodReturn(&expected_return_value);

    if (!ValueMatches(*expected_return_value, *return_value,
                      event->new_shared_objects())) {
      SetConflictDetected("Return value from self method call doesn't match "
                          "expected value.");
      return;
    }
  }
}

void PlaybackThread::DoSubMethodCall(PeerObjectImpl* peer_object,
                                     const string& method_name,
                                     const vector<Value>& parameters,
                                     Value* return_value) {
  CHECK(!conflict_detected_.Get());

  if (!CheckNextEventType(CommittedEvent::SUB_METHOD_CALL)) {
    return;
  }

  {
    SharedObject* callee = nullptr;
    const string* expected_method_name = nullptr;
    const vector<CommittedValue>* expected_parameters = nullptr;

    const CommittedEvent* const event = GetNextEvent();
    event->GetSubMethodCall(&callee, &expected_method_name,
                            &expected_parameters);

    if (callee == shared_object_) {
      SetConflictDetected("Callee is the same as caller, but a self method "
                          "call was not expected.");
      return;
    }

    if (!MethodCallMatches(callee, *expected_method_name, *expected_parameters,
                           peer_object, method_name, parameters,
                           event->new_shared_objects())) {
      SetConflictDetected("Sub method call doesn't match expected method "
                          "call.");
      return;
    }
  }

  while (HasNextEvent() && PeekNextEventType() == CommittedEvent::METHOD_CALL) {
    DoMethodCall();

    if (conflict_detected_.Get()) {
      return;
    }
  }

  if (!CheckNextEventType(CommittedEvent::SUB_METHOD_RETURN)) {
    return;
  }

  {
    SharedObject* callee = nullptr;
    const CommittedValue* expected_return_value = nullptr;

    GetNextEvent()->GetSubMethodReturn(&callee, &expected_return_value);

    ConvertCommittedValueToValue(*expected_return_value, return_value);
  }
}

bool PlaybackThread::HasNextEvent() {
  for (;;) {
    // Move to the next event in the queue.
    while (!event_queue_.HasNext()) {
      if (state_.Mutate(&PlaybackThread::ChangeToPausedAndWaitForRunning) ==
              STOPPING) {
        return false;
      }

      event_queue_.MoveToNextSequence();
    }

    const CommittedEvent* const event = event_queue_.PeekNext();

    if (event->type() == CommittedEvent::OBJECT_CREATION) {
      if (live_object_.get() == nullptr) {
        // The live object hasn't been created yet. Create it from the
        // OBJECT_CREATION event.

        shared_ptr<const LiveObject> new_live_object;
        event->GetObjectCreation(&new_live_object);

        live_object_ = new_live_object->Clone();
      }
    } else {
      if (live_object_.get() != nullptr) {
        return true;
      }
    }

    event_queue_.GetNext();
  }
}

CommittedEvent::Type PlaybackThread::PeekNextEventType() {
  CHECK(HasNextEvent());
  return event_queue_.PeekNext()->type();
}

const CommittedEvent* PlaybackThread::GetNextEvent() {
  CHECK(HasNextEvent());
  return event_queue_.GetNext();
}

bool PlaybackThread::CheckNextEventType(
    CommittedEvent::Type actual_event_type) {
  CHECK(!conflict_detected_.Get());

  if (!HasNextEvent()) {
    return false;
  }

  const CommittedEvent::Type expected_event_type = PeekNextEventType();

  if (expected_event_type != actual_event_type) {
    string description;
    SStringPrintf(&description, "Expected event type %s but received %s.",
                  CommittedEvent::GetTypeString(expected_event_type).c_str(),
                  CommittedEvent::GetTypeString(actual_event_type).c_str());

    SetConflictDetected(description);
    return false;
  }

  return true;
}

bool PlaybackThread::MethodCallMatches(
    SharedObject* expected_shared_object,
    const string& expected_method_name,
    const vector<CommittedValue>& expected_parameters,
    PeerObjectImpl* peer_object,
    const string& method_name,
    const vector<Value>& parameters,
    const unordered_set<SharedObject*>& new_shared_objects) {
  CHECK(peer_object != nullptr);

  if (!ObjectMatches(expected_shared_object, peer_object, new_shared_objects)) {
    VLOG(2) << "Objects don't match.";
    return false;
  }

  if (expected_method_name != method_name) {
    VLOG(2) << "Method names don't match (\"" << CEscape(expected_method_name)
            << "\" != \"" << CEscape(method_name) << "\").";
    return false;
  }

  if (expected_parameters.size() != parameters.size()) {
    VLOG(2) << "Parameter counts don't match (" << expected_parameters.size()
            << " != " << parameters.size() << ").";
    return false;
  }

  for (vector<CommittedValue>::size_type i = 0; i < expected_parameters.size();
       ++i) {
    const CommittedValue& expected_parameter = expected_parameters[i];

    if (!ValueMatches(expected_parameter, parameters[i], new_shared_objects)) {
      VLOG(2) << "Parameter " << i << ": values don't match.";
      return false;
    }
  }

  return true;
}

#define COMPARE_FIELDS(enum_const, getter_method) \
  case CommittedValue::enum_const: \
    return pending_value_type == Value::enum_const && \
        committed_value.getter_method() == pending_value.getter_method();

bool PlaybackThread::ValueMatches(
    const CommittedValue& committed_value, const Value& pending_value,
    const unordered_set<SharedObject*>& new_shared_objects) {
  if (committed_value.local_type() != pending_value.local_type()) {
    return false;
  }

  const CommittedValue::Type committed_value_type = committed_value.type();
  const Value::Type pending_value_type = pending_value.type();

  switch (committed_value_type) {
    case CommittedValue::EMPTY:
      return pending_value_type == Value::EMPTY;

    COMPARE_FIELDS(DOUBLE, double_value);
    COMPARE_FIELDS(FLOAT, float_value);
    COMPARE_FIELDS(INT64, int64_value);
    COMPARE_FIELDS(UINT64, uint64_value);
    COMPARE_FIELDS(BOOL, bool_value);
    COMPARE_FIELDS(STRING, string_value);
    COMPARE_FIELDS(BYTES, bytes_value);

    case CommittedValue::SHARED_OBJECT:
      return pending_value_type == Value::PEER_OBJECT &&
          ObjectMatches(
              committed_value.shared_object(),
              static_cast<PeerObjectImpl*>(pending_value.peer_object()),
              new_shared_objects);

    default:
      LOG(FATAL) << "Unexpected committed value type: "
                 << static_cast<int>(committed_value_type);
  }

  return false;
}

#undef COMPARE_FIELDS

bool PlaybackThread::ObjectMatches(
    SharedObject* shared_object, PeerObjectImpl* peer_object,
    const unordered_set<SharedObject*>& new_shared_objects) {
  CHECK(shared_object != nullptr);
  CHECK(peer_object != nullptr);

  const bool shared_object_is_new =
      (new_shared_objects.find(shared_object) != new_shared_objects.end());

  const unordered_set<PeerObjectImpl*>::iterator unbound_it =
      unbound_peer_objects_.find(peer_object);
  const bool peer_object_is_unbound =
      (unbound_it != unbound_peer_objects_.end());

  if (shared_object_is_new && peer_object_is_unbound) {
    const pair<unordered_map<SharedObject*, PeerObjectImpl*>::iterator, bool>
        insert_result = new_peer_objects_->emplace(shared_object, nullptr);

    if (!insert_result.second) {
      return false;
    }

    insert_result.first->second = peer_object;
    unbound_peer_objects_.erase(unbound_it);

    return true;
  }

  const unordered_map<SharedObject*, PeerObjectImpl*>::const_iterator
      new_peer_object_it = new_peer_objects_->find(shared_object);

  if (new_peer_object_it != new_peer_objects_->end() &&
      new_peer_object_it->second == peer_object) {
    return true;
  }

  return shared_object->HasPeerObject(peer_object);
}

void PlaybackThread::SetConflictDetected(const string& description) {
  if (FLAGS_treat_conflicts_as_fatal_for_debugging) {
    LOG(FATAL) << "CONFLICT: " << description;
  } else {
    VLOG(1) << "CONFLICT: " << description;
  }

  conflict_detected_.Set(true);
}

PeerObject* PlaybackThread::CreatePeerObject(
    LocalObject* initial_version, const string& name, bool versioned) {
  CHECK(initial_version != nullptr);

  delete initial_version;

  if (name.empty()) {
    if (transaction_store_->delay_object_binding() ||
        conflict_detected_.Get() ||
        !CheckNextEventType(CommittedEvent::SUB_OBJECT_CREATION)) {
      PeerObjectImpl* const peer_object =
          transaction_store_->CreateUnboundPeerObject(versioned);
      CHECK(unbound_peer_objects_.insert(peer_object).second);
      return peer_object;
    } else {
      const unordered_set<SharedObject*>& new_shared_objects =
          GetNextEvent()->new_shared_objects();
      CHECK_EQ(new_shared_objects.size(), 1u);
      SharedObject* const shared_object = *new_shared_objects.begin();
      return shared_object->GetOrCreatePeerObject(versioned);
    }
  } else {
    return transaction_store_->CreateBoundPeerObject(name, versioned);
  }
}

bool PlaybackThread::BeginTransaction() {
  if (conflict_detected_.Get() ||
      !CheckNextEventType(CommittedEvent::BEGIN_TRANSACTION)) {
    return false;
  }

  GetNextEvent();
  return HasNextEvent();
}

bool PlaybackThread::EndTransaction() {
  if (conflict_detected_.Get() ||
      !CheckNextEventType(CommittedEvent::END_TRANSACTION)) {
    return false;
  }

  GetNextEvent();
  return HasNextEvent();
}

PeerObject* PlaybackThread::CreateVersionedPeerObject(
    VersionedLocalObject* initial_version, const string& name) {
  return CreatePeerObject(initial_version, name, true);
}

PeerObject* PlaybackThread::CreateUnversionedPeerObject(
    UnversionedLocalObject* initial_version, const string& name) {
  return CreatePeerObject(initial_version, name, false);
}

bool PlaybackThread::CallMethod(PeerObject* peer_object,
                                const string& method_name,
                                const vector<Value>& parameters,
                                Value* return_value) {
  CHECK(!method_name.empty());

  if (conflict_detected_.Get() || !HasNextEvent()) {
    return false;
  }

  PeerObjectImpl* const peer_object_impl = static_cast<PeerObjectImpl*>(
      peer_object);

  if (shared_object_->HasPeerObject(peer_object_impl)) {
    DoSelfMethodCall(peer_object_impl, method_name, parameters, return_value);
  } else {
    DoSubMethodCall(peer_object_impl, method_name, parameters, return_value);
  }

  return !conflict_detected_.Get() && HasNextEvent();
}

bool PlaybackThread::ObjectsAreEquivalent(const PeerObject* a,
                                          const PeerObject* b) const {
  return transaction_store_->ObjectsAreEquivalent(
      static_cast<const PeerObjectImpl*>(a),
      static_cast<const PeerObjectImpl*>(b));
}

// static
void* PlaybackThread::ReplayThreadMain(void* playback_thread_raw) {
  CHECK(playback_thread_raw != nullptr);
  static_cast<PlaybackThread*>(playback_thread_raw)->ReplayEvents();
  return nullptr;
}

// static
void PlaybackThread::ChangeRunningToPaused(
    StateVariableInternalInterface* state_variable) {
  CHECK(state_variable != nullptr);

  if (state_variable->MatchesStateMask_Locked(RUNNING)) {
    state_variable->ChangeState_Locked(PAUSED);
  }
}

// static
void PlaybackThread::ChangePausedToRunning(
    StateVariableInternalInterface* state_variable) {
  CHECK(state_variable != nullptr);

  if (state_variable->MatchesStateMask_Locked(PAUSED)) {
    state_variable->ChangeState_Locked(RUNNING);
  }
}

// static
void PlaybackThread::ChangeToPausedAndWaitForRunning(
    StateVariableInternalInterface* state_variable) {
  CHECK(state_variable != nullptr);

  if (state_variable->MatchesStateMask_Locked(RUNNING)) {
    state_variable->ChangeState_Locked(PAUSED);
  }

  state_variable->WaitForState_Locked(RUNNING | STOPPING);
}

// static
void PlaybackThread::WaitForPausedAndChangeToStopping(
    StateVariableInternalInterface* state_variable) {
  CHECK(state_variable != nullptr);

  state_variable->WaitForState_Locked(PAUSED);
  state_variable->ChangeState_Locked(STOPPING);
}

}  // namespace peer
}  // namespace floating_temple
