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

#include "c_harness/proxy_interpreter.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "base/logging.h"
#include "c_harness/proxy_local_object.h"
#include "c_harness/types.h"
#include "include/c++/value.h"
#include "include/c/interpreter.h"
#include "include/c/peer.h"
#include "include/c/value.h"
#include "util/state_variable.h"

using std::size_t;
using std::string;
using std::unique_ptr;
using std::vector;

namespace floating_temple {
namespace c_harness {

__thread floatingtemple_Interpreter* ProxyInterpreter::interpreter_ = NULL;

ProxyInterpreter::ProxyInterpreter()
    : state_(START),
      callback_type_(NONE),
      const_local_object_(NULL),
      buffer_(NULL),
      buffer_size_(0),
      serialization_context_(NULL),
      const_buffer_(NULL),
      deserialization_context_(NULL),
      local_object_(NULL),
      thread_(NULL),
      peer_object_(NULL),
      method_name_(NULL),
      parameter_count_(0),
      parameters_(NULL),
      return_value_(NULL),
      returned_local_object_(NULL),
      returned_byte_count_(0) {
  state_.AddStateTransition(START, SETTING_PARAMETERS);
  state_.AddStateTransition(SETTING_PARAMETERS, PARAMETERS_SET);
  state_.AddStateTransition(PARAMETERS_SET, CALLBACK_EXECUTING);
  state_.AddStateTransition(CALLBACK_EXECUTING, CALLBACK_RETURNED);
  state_.AddStateTransition(CALLBACK_RETURNED, START);
}

LocalObject* ProxyInterpreter::CreateProxyLocalObject(
    floatingtemple_LocalObject* local_object) {
  return new ProxyLocalObject(this, local_object);
}

bool ProxyInterpreter::PollForCallback(
    floatingtemple_Interpreter* interpreter) {
  if (state_.Mutate(&ProxyInterpreter::
                    WaitForParametersSetAndChangeToCallbackExecuting) !=
      CALLBACK_EXECUTING) {
    return false;
  }

  ExecuteCallback(interpreter);
  state_.ChangeState(CALLBACK_RETURNED);

  return true;
}

floatingtemple_Interpreter* ProxyInterpreter::SetInterpreterForCurrentThread(
    floatingtemple_Interpreter* interpreter) {
  floatingtemple_Interpreter* const old_interpreter = interpreter_;
  interpreter_ = interpreter;
  return old_interpreter;
}

LocalObject* ProxyInterpreter::DeserializeObject(
    const void* buffer, size_t buffer_size, DeserializationContext* context) {
  CHECK(context != NULL);

  floatingtemple_DeserializationContext context_struct;
  context_struct.context = context;

  floatingtemple_LocalObject* new_local_object = NULL;

  if (interpreter_ == NULL) {
    EnterMethod(DESERIALIZE_OBJECT);

    const_buffer_ = buffer;
    buffer_size_ = buffer_size;
    deserialization_context_ = &context_struct;

    WaitForCallback();

    new_local_object = returned_local_object_;

    LeaveMethod();
  } else {
    new_local_object = interpreter_->deserialize_object(buffer, buffer_size,
                                                        &context_struct);
  }

  return new ProxyLocalObject(this, new_local_object);
}

void ProxyInterpreter::ExecuteCallback(
    floatingtemple_Interpreter* interpreter) {
  CHECK(interpreter != NULL);

  switch (callback_type_) {
    case NONE:
      LOG(FATAL) << "Callback type was not set.";
      break;

    case CLONE_LOCAL_OBJECT:
      returned_local_object_ = (*interpreter->clone_local_object)(
          const_local_object_);
      break;

    case SERIALIZE_LOCAL_OBJECT:
      returned_byte_count_ = (*interpreter->serialize_local_object)(
          const_local_object_, buffer_, buffer_size_, serialization_context_);
      break;

    case DESERIALIZE_OBJECT:
      returned_local_object_ = (*interpreter->deserialize_object)(
          const_buffer_, buffer_size_, deserialization_context_);
      break;

    case FREE_LOCAL_OBJECT:
      (*interpreter->free_local_object)(local_object_);
      break;

    case INVOKE_METHOD:
      (*interpreter->invoke_method)(local_object_, thread_, peer_object_,
                                    method_name_, parameter_count_, parameters_,
                                    return_value_);
      break;

    default:
      LOG(FATAL) << "Invalid callback type: "
                 << static_cast<int>(callback_type_);
  }
}

void ProxyInterpreter::EnterMethod(CallbackType callback_type) {
  state_.Mutate(&ProxyInterpreter::WaitForStartAndChangeToSettingParameters);
  callback_type_ = callback_type;
}

void ProxyInterpreter::WaitForCallback() {
  state_.Mutate(
      &ProxyInterpreter::ChangeToParametersSetAndWaitForCallbackReturned);
}

void ProxyInterpreter::LeaveMethod() {
  callback_type_ = NONE;
  const_local_object_ = NULL;
  buffer_ = NULL;
  buffer_size_ = 0;
  serialization_context_ = NULL;
  const_buffer_ = NULL;
  deserialization_context_ = NULL;
  local_object_ = NULL;
  thread_ = NULL;
  peer_object_ = NULL;
  method_name_ = NULL;
  parameter_count_ = 0;
  parameters_ = NULL;
  return_value_ = NULL;
  returned_local_object_ = NULL;
  returned_byte_count_ = 0;

  state_.ChangeState(START);
}

LocalObject* ProxyInterpreter::CloneLocalObject(
    const floatingtemple_LocalObject* local_object) {
  floatingtemple_LocalObject* new_local_object = NULL;

  if (interpreter_ == NULL) {
    EnterMethod(CLONE_LOCAL_OBJECT);

    const_local_object_ = local_object;

    WaitForCallback();

    new_local_object = returned_local_object_;

    LeaveMethod();
  } else {
    new_local_object = interpreter_->clone_local_object(local_object);
  }

  return new ProxyLocalObject(this, new_local_object);
}

size_t ProxyInterpreter::SerializeLocalObject(
    const floatingtemple_LocalObject* local_object,
    void* buffer,
    size_t buffer_size,
    SerializationContext* context) {
  CHECK(context != NULL);

  floatingtemple_SerializationContext context_struct;
  context_struct.context = context;

  size_t count = 0;

  if (interpreter_ == NULL) {
    EnterMethod(SERIALIZE_LOCAL_OBJECT);

    const_local_object_ = local_object;
    buffer_ = buffer;
    buffer_size_ = buffer_size;
    serialization_context_ = &context_struct;

    WaitForCallback();

    count = returned_byte_count_;

    LeaveMethod();
  } else {
    count = interpreter_->serialize_local_object(local_object, buffer,
                                                 buffer_size, &context_struct);
  }

  return count;
}

void ProxyInterpreter::InvokeMethodOnLocalObject(
    floatingtemple_LocalObject* local_object,
    Thread* thread,
    PeerObject* peer_object,
    const string& method_name,
    const vector<Value>& parameters,
    Value* return_value) {
  CHECK(thread != NULL);

  floatingtemple_Thread thread_struct;
  thread_struct.proxy_interpreter = this;
  thread_struct.thread = thread;

  const vector<Value>::size_type parameter_count = parameters.size();
  unique_ptr<floatingtemple_Value[]> parameter_array(
      new floatingtemple_Value[parameter_count]);

  for (vector<Value>::size_type i = 0; i < parameter_count; ++i) {
    floatingtemple_Value* const value_struct = &parameter_array[i];
    floatingtemple_InitValue(value_struct);
    *reinterpret_cast<Value*>(value_struct) = parameters[i];
  }

  if (interpreter_ == NULL) {
    EnterMethod(INVOKE_METHOD);

    local_object_ = local_object;
    thread_ = &thread_struct;
    peer_object_ = reinterpret_cast<floatingtemple_PeerObject*>(peer_object);
    method_name_ = method_name.c_str();
    parameter_count_ = static_cast<int>(parameter_count);
    parameters_ = parameter_array.get();
    return_value_ = reinterpret_cast<floatingtemple_Value*>(return_value);

    WaitForCallback();
    LeaveMethod();
  } else {
    interpreter_->invoke_method(
        local_object, &thread_struct,
        reinterpret_cast<floatingtemple_PeerObject*>(peer_object),
        method_name.c_str(), static_cast<int>(parameter_count),
        parameter_array.get(),
        reinterpret_cast<floatingtemple_Value*>(return_value));
  }

  for (vector<Value>::size_type i = 0; i < parameter_count; ++i) {
    floatingtemple_DestroyValue(&parameter_array[i]);
  }
}

void ProxyInterpreter::FreeLocalObject(
    floatingtemple_LocalObject* local_object) {
  if (interpreter_ == NULL) {
    EnterMethod(FREE_LOCAL_OBJECT);

    local_object_ = local_object;

    WaitForCallback();
    LeaveMethod();
  } else {
    interpreter_->free_local_object(local_object);
  }
}

// static
void ProxyInterpreter::WaitForStartAndChangeToSettingParameters(
    StateVariableInternalInterface* state_variable) {
  CHECK(state_variable != NULL);

  state_variable->WaitForState_Locked(START);
  state_variable->ChangeState_Locked(SETTING_PARAMETERS);
}

// static
void ProxyInterpreter::ChangeToParametersSetAndWaitForCallbackReturned(
    StateVariableInternalInterface* state_variable) {
  CHECK(state_variable != NULL);

  state_variable->ChangeState_Locked(PARAMETERS_SET);
  state_variable->WaitForState_Locked(CALLBACK_RETURNED);
}

// TODO(dss): Rename this method.
// static
void ProxyInterpreter::WaitForParametersSetAndChangeToCallbackExecuting(
    StateVariableInternalInterface* state_variable) {
  CHECK(state_variable != NULL);

  if (state_variable->MatchesStateMask_Locked(PARAMETERS_SET)) {
    state_variable->ChangeState_Locked(CALLBACK_EXECUTING);
  }
}

}  // namespace c_harness
}  // namespace floating_temple
