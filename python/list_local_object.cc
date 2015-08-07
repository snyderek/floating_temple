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

#include "python/list_local_object.h"

#include "third_party/Python-3.4.2/Include/Python.h"

#include <string>
#include <vector>

#include "base/logging.h"
#include "include/c++/deserialization_context.h"
#include "include/c++/object_reference.h"
#include "include/c++/serialization_context.h"
#include "python/interpreter_impl.h"
#include "python/proto/serialization.pb.h"
#include "python/python_gil_lock.h"
#include "python/versioned_local_object_impl.h"
#include "util/dump_context.h"

using std::vector;

namespace floating_temple {
namespace python {

ListLocalObject::ListLocalObject(PyObject* py_list_object)
    : VersionedLocalObjectImpl(CHECK_NOTNULL(py_list_object)) {
}

VersionedLocalObject* ListLocalObject::Clone() const {
  PyObject* const py_list = py_object();
  PyObject* new_py_list = nullptr;

  {
    PythonGilLock lock;
    const Py_ssize_t length = PyList_Size(py_list);
    CHECK_GE(length, 0);
    new_py_list = PyList_GetSlice(py_list, 0, length);
  }

  return new ListLocalObject(new_py_list);
}

void ListLocalObject::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  InterpreterImpl* const interpreter = InterpreterImpl::instance();
  PyObject* const py_list = py_object();

  vector<const ObjectReference*> object_references;
  {
    PythonGilLock lock;

    const Py_ssize_t length = PyList_Size(py_list);
    CHECK_GE(length, 0);
    object_references.reserve(
        static_cast<vector<ObjectReference*>::size_type>(length));

    for (Py_ssize_t i = 0; i < length; ++i) {
      PyObject* const py_item = PyList_GetItem(py_list, i);
      const ObjectReference* const object_reference =
          interpreter->PyProxyObjectToObjectReference(py_item);
      object_references.push_back(object_reference);
    }
  }

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("ListLocalObject");

  dc->AddString("items");
  dc->BeginList();
  for (const ObjectReference* const object_reference : object_references) {
    object_reference->Dump(dc);
  }
  dc->End();

  dc->End();
}

// static
ListLocalObject* ListLocalObject::ParseListProto(
    const SequenceProto& list_proto, DeserializationContext* context) {
  CHECK(context != nullptr);

  InterpreterImpl* const interpreter = InterpreterImpl::instance();
  const int item_count = list_proto.item_size();

  PyObject* py_list = nullptr;
  {
    PythonGilLock lock;

    py_list = PyList_New(static_cast<Py_ssize_t>(item_count));
    CHECK(py_list != nullptr);

    for (int i = 0; i < item_count; ++i) {
      const int object_index = static_cast<int>(
          list_proto.item(i).object_index());
      ObjectReference* const object_reference =
          context->GetObjectReferenceByIndex(object_index);
      PyObject* const py_item = interpreter->ObjectReferenceToPyProxyObject(
          object_reference);
      CHECK_EQ(PyList_SetItem(py_list, static_cast<Py_ssize_t>(i), py_item), 0);
    }
  }

  return new ListLocalObject(py_list);
}

void ListLocalObject::PopulateObjectProto(ObjectProto* object_proto,
                                          SerializationContext* context) const {
  CHECK(object_proto != nullptr);
  CHECK(context != nullptr);

  InterpreterImpl* const interpreter = InterpreterImpl::instance();
  PyObject* const py_list = py_object();
  SequenceProto* const list_proto = object_proto->mutable_list_object();

  {
    PythonGilLock lock;

    const Py_ssize_t length = PyList_Size(py_list);

    for (Py_ssize_t i = 0; i < length; ++i) {
      PyObject* const py_item = PyList_GetItem(py_list, i);
      ObjectReference* const object_reference =
          interpreter->PyProxyObjectToObjectReference(py_item);
      const int object_index = context->GetIndexForObjectReference(
          object_reference);
      list_proto->add_item()->set_object_index(object_index);
    }
  }
}

}  // namespace python
}  // namespace floating_temple
