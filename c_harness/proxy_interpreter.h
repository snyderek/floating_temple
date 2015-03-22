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

#ifndef C_HARNESS_PROXY_INTERPRETER_H_
#define C_HARNESS_PROXY_INTERPRETER_H_

#include <cstddef>

#include "base/macros.h"
#include "c_harness/proxy_interpreter_internal_interface.h"
#include "include/c++/interpreter.h"
#include "include/c/interpreter.h"
#include "include/c/peer.h"
#include "include/c/value.h"
#include "util/state_variable.h"

namespace floating_temple {

class LocalObject;
class StateVariableInternalInterface;

namespace c_harness {

class ProxyInterpreter : public Interpreter,
                         private ProxyInterpreterInternalInterface {
 public:
  ProxyInterpreter();

  // The caller must take ownership of the returned LocalObject instance.
  LocalObject* CreateProxyLocalObject(floatingtemple_LocalObject* local_object);

  bool PollForCallback(floatingtemple_Interpreter* interpreter);

  floatingtemple_Interpreter* SetInterpreterForCurrentThread(
      floatingtemple_Interpreter* interpreter);

  LocalObject* DeserializeObject(const void* buffer, std::size_t buffer_size,
                                 DeserializationContext* context) override;

 private:
  enum {
    START = 0x1,
    SETTING_PARAMETERS = 0x2,
    PARAMETERS_SET = 0x4,
    CALLBACK_EXECUTING = 0x8,
    CALLBACK_RETURNED = 0x10
  };

  enum CallbackType {
    NONE,
    CLONE_LOCAL_OBJECT,
    SERIALIZE_LOCAL_OBJECT,
    DESERIALIZE_OBJECT,
    FREE_LOCAL_OBJECT,
    INVOKE_METHOD
  };

  void ExecuteCallback(floatingtemple_Interpreter* interpreter);

  void EnterMethod(CallbackType callback_type);
  void WaitForCallback();
  void LeaveMethod();

  LocalObject* CloneLocalObject(
      const floatingtemple_LocalObject* local_object) override;
  std::size_t SerializeLocalObject(
      const floatingtemple_LocalObject* local_object,
      void* buffer,
      std::size_t buffer_size,
      SerializationContext* context) override;
  void InvokeMethodOnLocalObject(
      floatingtemple_LocalObject* local_object,
      Thread* thread,
      PeerObject* peer_object,
      const std::string& method_name,
      const std::vector<Value>& parameters,
      Value* return_value) override;
  void FreeLocalObject(floatingtemple_LocalObject* local_object) override;

  static void WaitForStartAndChangeToSettingParameters(
      StateVariableInternalInterface* state_variable);
  static void ChangeToParametersSetAndWaitForCallbackReturned(
      StateVariableInternalInterface* state_variable);
  static void WaitForParametersSetAndChangeToCallbackExecuting(
      StateVariableInternalInterface* state_variable);

  StateVariable state_;

  CallbackType callback_type_;

  // Callback parameters
  const floatingtemple_LocalObject* const_local_object_;
  void* buffer_;
  std::size_t buffer_size_;
  floatingtemple_SerializationContext* serialization_context_;
  const void* const_buffer_;
  floatingtemple_DeserializationContext* deserialization_context_;
  floatingtemple_LocalObject* local_object_;
  floatingtemple_Thread* thread_;
  floatingtemple_PeerObject* peer_object_;
  const char* method_name_;
  int parameter_count_;
  const floatingtemple_Value* parameters_;
  floatingtemple_Value* return_value_;

  // Callback return values
  floatingtemple_LocalObject* returned_local_object_;
  std::size_t returned_byte_count_;

  static __thread floatingtemple_Interpreter* interpreter_;

  DISALLOW_COPY_AND_ASSIGN(ProxyInterpreter);
};

}  // namespace c_harness
}  // namespace floating_temple

#endif  // C_HARNESS_PROXY_INTERPRETER_H_
