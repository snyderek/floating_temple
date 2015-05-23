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

#ifndef C_HARNESS_PROXY_LOCAL_OBJECT_H_
#define C_HARNESS_PROXY_LOCAL_OBJECT_H_

#include "base/macros.h"
#include "include/c++/versioned_local_object.h"
#include "include/c/interpreter.h"

namespace floating_temple {
namespace c_harness {

class ProxyInterpreterInternalInterface;

class ProxyLocalObject : public VersionedLocalObject {
 public:
  ProxyLocalObject(ProxyInterpreterInternalInterface* proxy_interpreter,
                   floatingtemple_LocalObject* local_object);
  ~ProxyLocalObject() override;

  VersionedLocalObject* Clone() const override;
  std::size_t Serialize(void* buffer, std::size_t buffer_size,
                        SerializationContext* context) const override;
  void InvokeMethod(Thread* thread,
                    PeerObject* peer_object,
                    const std::string& method_name,
                    const std::vector<Value>& parameters,
                    Value* return_value) override;
  std::string Dump() const override;

 private:
  ProxyInterpreterInternalInterface* const proxy_interpreter_;
  floatingtemple_LocalObject* const local_object_;

  DISALLOW_COPY_AND_ASSIGN(ProxyLocalObject);
};

}  // namespace c_harness
}  // namespace floating_temple

#endif  // C_HARNESS_PROXY_LOCAL_OBJECT_H_
