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

#include "fake_engine/fake_method_context.h"

#include <memory>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/string_printf.h"
#include "fake_engine/fake_object_reference.h"
#include "include/c++/local_object.h"
#include "util/dump_context_impl.h"

using std::string;
using std::unique_ptr;
using std::vector;

namespace floating_temple {

FakeMethodContext::FakeMethodContext()
    : transaction_depth_(0) {
}

FakeMethodContext::~FakeMethodContext() {
  for (unique_ptr<ObjectReference>& object_reference : object_references_) {
    VLOG(1) << "Deleting object reference "
            << StringPrintf("%p", object_reference.get());
    object_reference.reset(nullptr);
  }
}

bool FakeMethodContext::BeginTransaction() {
  ++transaction_depth_;
  CHECK_GT(transaction_depth_, 0);
  return true;
}

bool FakeMethodContext::EndTransaction() {
  CHECK_GT(transaction_depth_, 0);
  --transaction_depth_;
  return true;
}

ObjectReference* FakeMethodContext::CreateObject(LocalObject* initial_version,
                                                 const string& name) {
  // TODO(dss): If an object with the given name was already created, return a
  // pointer to the existing ObjectReference instance.

  // TODO(dss): Implement garbage collection.

  ObjectReference* const object_reference = new FakeObjectReference(
      initial_version);
  VLOG(1) << "New object reference: " << StringPrintf("%p", object_reference);
  VLOG(1) << "object_reference: " << GetJsonString(*object_reference);
  object_references_.emplace_back(object_reference);
  return object_reference;
}

bool FakeMethodContext::CallMethod(ObjectReference* object_reference,
                                   const string& method_name,
                                   const vector<Value>& parameters,
                                   Value* return_value) {
  CHECK(object_reference != nullptr);
  CHECK(!method_name.empty());
  CHECK(return_value != nullptr);

  VLOG(1) << "Calling method on object reference: "
          << StringPrintf("%p", object_reference);
  VLOG(1) << "object_reference: " << GetJsonString(*object_reference);

  FakeObjectReference* const fake_object_reference =
      static_cast<FakeObjectReference*>(object_reference);
  LocalObject* const local_object = fake_object_reference->local_object();

  VLOG(1) << "local_object: " << GetJsonString(*local_object);

  local_object->InvokeMethod(this, object_reference, method_name, parameters,
                             return_value);

  return true;
}

bool FakeMethodContext::ObjectsAreIdentical(const ObjectReference* a,
                                            const ObjectReference* b) const {
  CHECK(a != nullptr);
  CHECK(b != nullptr);

  return a == b;
}

}  // namespace floating_temple
