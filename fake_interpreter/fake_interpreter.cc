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

#include "fake_interpreter/fake_interpreter.h"

#include <cstddef>
#include <string>

#include "base/logging.h"
#include "fake_interpreter/fake_local_object.h"

using std::size_t;
using std::string;

namespace floating_temple {

FakeInterpreter::FakeInterpreter() {
}

LocalObject* FakeInterpreter::DeserializeObject(
    const void* buffer, size_t buffer_size, DeserializationContext* context) {
  CHECK(buffer != NULL);

  const string s(static_cast<const char*>(buffer),
                 static_cast<string::size_type>(buffer_size));

  return new FakeLocalObject(s);
}

}  // namespace floating_temple