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

#include "fake_engine/fake_peer.h"

#include <string>
#include <vector>

#include "base/logging.h"
#include "fake_engine/fake_thread.h"
#include "include/c++/local_object.h"
#include "include/c++/value.h"

using std::string;
using std::vector;

namespace floating_temple {

class ObjectReference;

FakePeer::FakePeer() {
}

void FakePeer::RunProgram(LocalObject* local_object,
                          const string& method_name,
                          Value* return_value,
                          bool linger) {
  CHECK(local_object != nullptr);
  // TODO(dss): Simulate linger mode by just sleeping forever after the program
  // completes.
  CHECK(!linger) << "Linger mode isn't supported for the fake peer.";

  FakeThread thread;
  ObjectReference* const object_reference = thread.CreateObject(local_object,
                                                                "");

  local_object->InvokeMethod(&thread, object_reference, method_name,
                             vector<Value>(), return_value);
}

void FakePeer::Stop() {
}

}  // namespace floating_temple
