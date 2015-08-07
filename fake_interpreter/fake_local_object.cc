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

#include "fake_interpreter/fake_local_object.h"

#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

#include "base/escape.h"
#include "base/logging.h"
#include "include/c++/value.h"
#include "util/dump_context.h"

using std::memcpy;
using std::size_t;
using std::string;
using std::vector;

namespace floating_temple {

const int FakeVersionedLocalObject::kVoidLocalType = 0;
const int FakeVersionedLocalObject::kStringLocalType = 1;
const int FakeVersionedLocalObject::kObjectLocalType = 2;

const char FakeVersionedLocalObject::kSerializationPrefix[] =
    "FakeVersionedLocalObject:";

VersionedLocalObject* FakeVersionedLocalObject::Clone() const {
  return new FakeVersionedLocalObject(s_);
}

size_t FakeVersionedLocalObject::Serialize(
    void* buffer, size_t buffer_size, SerializationContext* context) const {
  string serialized_form = kSerializationPrefix;
  serialized_form += s_;

  const string::size_type length = serialized_form.length();

  if (static_cast<string::size_type>(buffer_size) >= length) {
    memcpy(buffer, serialized_form.data(), length);
  }

  return static_cast<size_t>(length);
}

void FakeVersionedLocalObject::InvokeMethod(Thread* thread,
                                            ObjectReference* object_reference,
                                            const string& method_name,
                                            const vector<Value>& parameters,
                                            Value* return_value) {
  CHECK(return_value != nullptr);

  VLOG(1) << "Applying method \"" << CEscape(method_name) << "\" on object "
          << this;

  if (method_name == "append") {
    CHECK_EQ(parameters.size(), 1u);
    const Value& parameter = parameters[0];
    CHECK_EQ(parameter.type(), Value::STRING);
    s_ += parameter.string_value();
    VLOG(2) << "s == \"" << CEscape(s_) << "\"";
    return_value->set_empty(kVoidLocalType);
  } else if (method_name == "clear") {
    CHECK_EQ(parameters.size(), 0u);
    s_.clear();
    return_value->set_empty(kVoidLocalType);
  } else if (method_name == "get") {
    CHECK_EQ(parameters.size(), 0u);
    return_value->set_string_value(kStringLocalType, s_);
  } else {
    LOG(FATAL) << "Unrecognized method name \"" << CEscape(method_name) << "\"";
  }
}

void FakeVersionedLocalObject::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);
  dc->AddString(s_);
}

}  // namespace floating_temple
