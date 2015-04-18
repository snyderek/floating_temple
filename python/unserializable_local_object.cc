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

#include "python/unserializable_local_object.h"

#include "third_party/Python-3.4.2/Include/Python.h"

#include <string>

#include "base/logging.h"
#include "python/local_object_impl.h"
#include "python/proto/serialization.pb.h"

using std::string;

namespace floating_temple {
namespace python {

UnserializableLocalObject::UnserializableLocalObject(PyObject* py_object)
    : LocalObjectImpl(CHECK_NOTNULL(py_object)) {
}

LocalObject* UnserializableLocalObject::Clone() const {
  return new UnserializableLocalObject(py_object());
}

string UnserializableLocalObject::Dump() const {
  return "{ \"type\": \"UnserializableLocalObject\" }";
}

void UnserializableLocalObject::PopulateObjectProto(
    ObjectProto* object_proto, SerializationContext* context) const {
  CHECK(object_proto != nullptr);

  const PyTypeObject* const py_type = Py_TYPE(py_object());
  CHECK(py_type != nullptr);
  const char* const type_name = py_type->tp_name;
  CHECK(type_name != nullptr);

  object_proto->mutable_unserializable_object()->set_type_name(type_name);
}

}  // namespace python
}  // namespace floating_temple
