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

#include "c_harness/proxy_local_object.h"

#include <cstddef>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/string_printf.h"
#include "c_harness/proxy_interpreter_internal_interface.h"

using std::size_t;
using std::string;
using std::vector;

namespace floating_temple {
namespace c_harness {

ProxyLocalObject::ProxyLocalObject(
    ProxyInterpreterInternalInterface* proxy_interpreter,
    floatingtemple_LocalObject* local_object)
    : proxy_interpreter_(CHECK_NOTNULL(proxy_interpreter)),
      local_object_(CHECK_NOTNULL(local_object)) {
}

ProxyLocalObject::~ProxyLocalObject() {
  proxy_interpreter_->FreeLocalObject(local_object_);
}

LocalObject* ProxyLocalObject::Clone() const {
  return proxy_interpreter_->CloneLocalObject(local_object_);
}

size_t ProxyLocalObject::Serialize(void* buffer, size_t buffer_size,
                                   SerializationContext* context) const {
  return proxy_interpreter_->SerializeLocalObject(local_object_, buffer,
                                                  buffer_size, context);
}

void ProxyLocalObject::InvokeMethod(Thread* thread,
                                    PeerObject* peer_object,
                                    const string& method_name,
                                    const vector<Value>& parameters,
                                    Value* return_value) {
  proxy_interpreter_->InvokeMethodOnLocalObject(local_object_, thread,
                                                peer_object, method_name,
                                                parameters, return_value);
}

string ProxyLocalObject::Dump() const {
  return StringPrintf("{ \"local_object\": \"%p\" }", local_object_);
}

}  // namespace c_harness
}  // namespace floating_temple
