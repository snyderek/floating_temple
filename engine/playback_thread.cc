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

#include "engine/playback_thread.h"

#include <pthread.h>

#include <memory>
#include <string>
#include <vector>

#include <gflags/gflags.h>

#include "base/escape.h"
#include "base/logging.h"
#include "base/string_printf.h"
#include "engine/committed_event.h"
#include "engine/event_matching.h"
#include "engine/event_queue.h"
#include "engine/live_object.h"
#include "engine/object_reference_impl.h"
#include "engine/shared_object.h"
#include "engine/transaction_store_internal_interface.h"
#include "include/c++/local_object.h"
#include "include/c++/value.h"
#include "util/bool_variable.h"
#include "util/state_variable.h"
#include "util/state_variable_internal_interface.h"

using std::shared_ptr;
using std::string;
using std::vector;

DEFINE_bool(treat_conflicts_as_fatal_for_debugging, false,
            "Log a FATAL error if a conflict occurs while replaying "
            "transactions on an object");

namespace floating_temple {
namespace engine {

PlaybackThread::PlaybackThread()
    : transaction_store_(nullptr),
      shared_object_(nullptr),
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

void PlaybackThread::Start(TransactionStoreInternalInterface* transaction_store,
                           SharedObject* shared_object,
                           const shared_ptr<LiveObject>& live_object) {
  CHECK(transaction_store != nullptr);
  CHECK(shared_object != nullptr);

  state_.ChangeState(STARTING);

  transaction_store_ = transaction_store;
  shared_object_ = shared_object;
  live_object_ = live_object;

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
    const string* method_name_temp = nullptr;
    const vector<Value>* committed_parameters = nullptr;

    GetNextEvent()->GetMethodCall(&method_name_temp, &committed_parameters);

    method_name = *method_name_temp;
    VLOG(2) << "method_name == \"" << CEscape(method_name) << "\"";

    parameters = *committed_parameters;
  }

  if (!HasNextEvent()) {
    return;
  }

  ObjectReferenceImpl* const object_reference =
      shared_object_->GetOrCreateObjectReference();

  Value return_value;
  live_object_->InvokeMethod(this, object_reference, method_name, parameters,
                             &return_value);

  if (conflict_detected_.Get() ||
      !CheckNextEventType(CommittedEvent::METHOD_RETURN)) {
    return;
  }

  {
    const Value* expected_return_value = nullptr;

    const CommittedEvent* const event = GetNextEvent();
    event->GetMethodReturn(&expected_return_value);

    if (!ValueMatches(*expected_return_value, return_value)) {
      SetConflictDetected("Return value doesn't match expected return value.");
      return;
    }
  }
}

void PlaybackThread::DoSelfMethodCall(ObjectReferenceImpl* object_reference,
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
    const vector<Value>* expected_parameters = nullptr;

    const CommittedEvent* const event = GetNextEvent();
    event->GetSelfMethodCall(&expected_method_name, &expected_parameters);

    if (!MethodCallMatches(shared_object_, *expected_method_name,
                           *expected_parameters, object_reference, method_name,
                           parameters)) {
      SetConflictDetected("Self method call doesn't match expected method "
                          "call.");
      return;
    }
  }

  if (!HasNextEvent()) {
    return;
  }

  live_object_->InvokeMethod(this, object_reference, method_name, parameters,
                             return_value);

  if (conflict_detected_.Get() ||
      !CheckNextEventType(CommittedEvent::SELF_METHOD_RETURN)) {
    return;
  }

  {
    const Value* expected_return_value = nullptr;

    const CommittedEvent* const event = GetNextEvent();
    event->GetSelfMethodReturn(&expected_return_value);

    if (!ValueMatches(*expected_return_value, *return_value)) {
      SetConflictDetected("Return value from self method call doesn't match "
                          "expected value.");
      return;
    }
  }
}

void PlaybackThread::DoSubMethodCall(ObjectReferenceImpl* object_reference,
                                     const string& method_name,
                                     const vector<Value>& parameters,
                                     Value* return_value) {
  CHECK(!conflict_detected_.Get());
  CHECK(return_value != nullptr);

  if (!CheckNextEventType(CommittedEvent::SUB_METHOD_CALL)) {
    return;
  }

  {
    ObjectReferenceImpl* callee = nullptr;
    const string* expected_method_name = nullptr;
    const vector<Value>* expected_parameters = nullptr;

    const CommittedEvent* const event = GetNextEvent();
    event->GetSubMethodCall(&callee, &expected_method_name,
                            &expected_parameters);

    SharedObject* const callee_shared_object = callee->shared_object();
    CHECK(callee_shared_object != nullptr);

    if (callee_shared_object == shared_object_) {
      SetConflictDetected("Callee is the same as caller, but a self method "
                          "call was not expected.");
      return;
    }

    if (!MethodCallMatches(callee_shared_object, *expected_method_name,
                           *expected_parameters, object_reference, method_name,
                           parameters)) {
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
    const Value* expected_return_value = nullptr;
    GetNextEvent()->GetSubMethodReturn(&expected_return_value);

    *return_value = *expected_return_value;
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

ObjectReferenceImpl* PlaybackThread::GetNewObjectReference(
    const string& new_object_name) {
  if (conflict_detected_.Get()) {
    return nullptr;
  }

  if (!CheckNextEventType(CommittedEvent::SUB_OBJECT_CREATION)) {
    return nullptr;
  }

  const string* expected_name = nullptr;
  ObjectReferenceImpl* new_object = nullptr;

  GetNextEvent()->GetSubObjectCreation(&expected_name, &new_object);

  if (new_object_name != *expected_name) {
    string description;
    SStringPrintf(
        &description,
        "Expected object \"%s\" to be created, but instead object \"%s\" was "
        "created.",
        CEscape(*expected_name).c_str(),
        CEscape(new_object_name).c_str());

    SetConflictDetected(description);

    return nullptr;
  }

  return new_object;
}

void PlaybackThread::SetConflictDetected(const string& description) {
  if (FLAGS_treat_conflicts_as_fatal_for_debugging) {
    LOG(FATAL) << "CONFLICT: " << description;
  } else {
    VLOG(1) << "CONFLICT: " << description;
  }

  conflict_detected_.Set(true);
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

ObjectReference* PlaybackThread::CreateObject(LocalObject* initial_version,
                                              const string& name) {
  CHECK(initial_version != nullptr);

  delete initial_version;

  ObjectReferenceImpl* const new_object = GetNewObjectReference(name);
  if (new_object != nullptr) {
    return new_object;
  }

  return transaction_store_->CreateBoundObjectReference(name);
}

bool PlaybackThread::CallMethod(ObjectReference* object_reference,
                                const string& method_name,
                                const vector<Value>& parameters,
                                Value* return_value) {
  CHECK(!method_name.empty());

  if (conflict_detected_.Get() || !HasNextEvent()) {
    return false;
  }

  ObjectReferenceImpl* const callee_object_reference =
      static_cast<ObjectReferenceImpl*>(object_reference);

  if (shared_object_->HasObjectReference(callee_object_reference)) {
    DoSelfMethodCall(callee_object_reference, method_name, parameters,
                     return_value);
  } else {
    DoSubMethodCall(callee_object_reference, method_name, parameters,
                    return_value);
  }

  return !conflict_detected_.Get() && HasNextEvent();
}

bool PlaybackThread::ObjectsAreIdentical(const ObjectReference* a,
                                         const ObjectReference* b) const {
  return transaction_store_->ObjectsAreIdentical(
      static_cast<const ObjectReferenceImpl*>(a),
      static_cast<const ObjectReferenceImpl*>(b));
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

}  // namespace engine
}  // namespace floating_temple
