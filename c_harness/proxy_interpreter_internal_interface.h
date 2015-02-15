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

#ifndef C_HARNESS_PROXY_INTERPRETER_INTERNAL_INTERFACE_H_
#define C_HARNESS_PROXY_INTERPRETER_INTERNAL_INTERFACE_H_

#include <cstddef>
#include <string>
#include <vector>

#include "include/c++/value.h"
#include "include/c/interpreter.h"

namespace floating_temple {

class LocalObject;
class PeerObject;
class SerializationContext;
class Thread;

namespace c_harness {

class ProxyInterpreterInternalInterface {
 public:
  virtual ~ProxyInterpreterInternalInterface() {}

  virtual LocalObject* CloneLocalObject(
      const floatingtemple_LocalObject* local_object) = 0;
  virtual std::size_t SerializeLocalObject(
      const floatingtemple_LocalObject* local_object,
      void* buffer,
      std::size_t buffer_size,
      SerializationContext* context) = 0;
  virtual void InvokeMethodOnLocalObject(
      floatingtemple_LocalObject* local_object,
      Thread* thread,
      PeerObject* peer_object,
      const std::string& method_name,
      const std::vector<Value>& parameters,
      Value* return_value) = 0;
  virtual void FreeLocalObject(floatingtemple_LocalObject* local_object) = 0;
};

}  // namespace c_harness
}  // namespace floating_temple

#endif  // C_HARNESS_PROXY_INTERPRETER_INTERNAL_INTERFACE_H_
