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

#include "base/logging.h"
#include "base/string_printf.h"
#include "include/c++/deserialization_context.h"
#include "include/c++/peer_object.h"
#include "include/c++/serialization_context.h"
#include "python/interpreter_impl.h"
#include "python/local_object_impl.h"
#include "python/proto/serialization.pb.h"
#include "python/python_gil_lock.h"

using std::string;

namespace floating_temple {
namespace python {

ListLocalObject::ListLocalObject(PyObject* py_list_object)
    : LocalObjectImpl(CHECK_NOTNULL(py_list_object)) {
}

LocalObject* ListLocalObject::Clone() const {
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

string ListLocalObject::Dump() const {
  InterpreterImpl* const interpreter = InterpreterImpl::instance();
  PyObject* const py_list = py_object();

  string items_string;
  {
    PythonGilLock lock;

    const Py_ssize_t length = PyList_Size(py_list);
    CHECK_GE(length, 0);

    if (length == 0) {
      items_string = "[]";
    } else {
      items_string = "[";

      for (Py_ssize_t i = 0; i < length; ++i) {
        if (i > 0) {
          items_string += ",";
        }

        PyObject* const py_item = PyList_GetItem(py_list, i);
        PeerObject* const peer_object = interpreter->PyObjectToPeerObject(
            py_item);

        StringAppendF(&items_string, " %s", peer_object->Dump().c_str());
      }

      items_string += " ]";
    }
  }

  return StringPrintf("{ \"type\": \"ListLocalObject\", \"items\": %s }",
                      items_string.c_str());
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
      PeerObject* const peer_object = context->GetPeerObjectByIndex(
          object_index);
      PyObject* const py_item = interpreter->PeerObjectToPyObject(peer_object);
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
      PeerObject* const peer_object = interpreter->PyObjectToPeerObject(
          py_item);
      const int object_index = context->GetIndexForPeerObject(peer_object);
      list_proto->add_item()->set_object_index(object_index);
    }
  }
}

}  // namespace python
}  // namespace floating_temple
