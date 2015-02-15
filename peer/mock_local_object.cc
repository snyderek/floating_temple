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

#include "peer/mock_local_object.h"

#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

#include "base/logging.h"
#include "include/c++/value.h"

using std::memcpy;
using std::size_t;
using std::string;
using std::vector;

namespace floating_temple {
namespace peer {

MockLocalObject::MockLocalObject(const MockLocalObjectCore* core)
    : core_(CHECK_NOTNULL(core)) {
}

LocalObject* MockLocalObject::Clone() const {
  return new MockLocalObject(core_);
}

size_t MockLocalObject::Serialize(void* buffer, size_t buffer_size,
                                  SerializationContext* context) const {
  CHECK(buffer != NULL);
  CHECK(context != NULL);

  const string data = core_->Serialize(context);
  const size_t data_size = static_cast<size_t>(data.length());

  if (data_size <= buffer_size) {
    memcpy(buffer, data.data(), data_size);
  }

  return data_size;
}

void MockLocalObject::InvokeMethod(Thread* thread,
                                   PeerObject* peer_object,
                                   const string& method_name,
                                   const vector<Value>& parameters,
                                   Value* return_value) {
  core_->InvokeMethod(thread, peer_object, method_name, parameters,
                      return_value);
}

string MockLocalObject::Dump() const {
  return "";
}

}  // namespace peer
}  // namespace floating_temple