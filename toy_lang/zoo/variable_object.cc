// Floating Temple
// Copyright 2016 Derek S. Snyder
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

#include "toy_lang/zoo/variable_object.h"

#include <string>
#include <vector>

#include "base/escape.h"
#include "base/logging.h"
#include "base/mutex.h"
#include "base/mutex_lock.h"
#include "include/c++/deserialization_context.h"
#include "include/c++/object_reference.h"
#include "include/c++/serialization_context.h"
#include "include/c++/value.h"
#include "toy_lang/proto/serialization.pb.h"
#include "util/dump_context.h"

using std::string;
using std::vector;

namespace floating_temple {
namespace toy_lang {

VariableObject::VariableObject(ObjectReference* object_reference)
    : object_reference_(object_reference) {
}

VersionedLocalObject* VariableObject::Clone() const {
  return new VariableObject(GetObjectReference());
}

void VariableObject::InvokeMethod(Thread* thread,
                                  ObjectReference* object_reference,
                                  const string& method_name,
                                  const vector<Value>& parameters,
                                  Value* return_value) {
  CHECK(return_value != nullptr);

  if (method_name == "get") {
    CHECK_EQ(parameters.size(), 0u);

    ObjectReference* const object_reference = GetObjectReference();
    CHECK(object_reference != nullptr) << "Variable is unset.";
    return_value->set_object_reference(0, object_reference);
  } else if (method_name == "set") {
    CHECK_EQ(parameters.size(), 1u);

    ObjectReference* const object_reference = parameters[0].object_reference();
    {
      MutexLock lock(&mu_);
      object_reference_ = object_reference;
    }

    return_value->set_empty(0);
  } else {
    LOG(FATAL) << "Unsupported method: \"" << CEscape(method_name) << "\"";
  }
}

void VariableObject::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  const ObjectReference* const object_reference = GetObjectReference();

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("VariableObject");

  dc->AddString("object_reference");
  if (object_reference == nullptr) {
    dc->AddNull();
  } else {
    object_reference->Dump(dc);
  }

  dc->End();
}

// static
VariableObject* VariableObject::ParseVariableProto(
    const VariableProto& variable_proto, DeserializationContext* context) {
  CHECK(context != nullptr);

  ObjectReference* object_reference = nullptr;
  if (variable_proto.has_object_index()) {
    object_reference = context->GetObjectReferenceByIndex(
        variable_proto.object_index());
  }

  return new VariableObject(object_reference);
}

void VariableObject::PopulateObjectProto(ObjectProto* object_proto,
                                         SerializationContext* context) const {
  VariableProto* const variable_proto = object_proto->mutable_variable_object();

  ObjectReference* const object_reference = GetObjectReference();
  if (object_reference != nullptr) {
    const int object_index = context->GetIndexForObjectReference(
        object_reference);
    variable_proto->set_object_index(object_index);
  }
}

ObjectReference* VariableObject::GetObjectReference() const {
  MutexLock lock(&mu_);
  return object_reference_;
}

}  // namespace toy_lang
}  // namespace floating_temple
