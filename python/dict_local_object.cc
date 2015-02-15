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

#include "python/dict_local_object.h"

#include "third_party/Python-3.4.2/Include/Python.h"

#include <cstddef>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/string_printf.h"
#include "include/c++/deserialization_context.h"
#include "include/c++/peer_object.h"
#include "include/c++/serialization_context.h"
#include "include/c++/value.h"
#include "python/local_object_impl.h"
#include "python/proto/serialization.pb.h"
#include "python/py_proxy_object.h"
#include "python/python_gil_lock.h"

using std::string;
using std::vector;

namespace floating_temple {
namespace python {

DictLocalObject::DictLocalObject(PyObject* py_dict_object)
    : LocalObjectImpl(CHECK_NOTNULL(py_dict_object)) {
}

LocalObject* DictLocalObject::Clone() const {
  PyObject* const py_dict = py_object();
  PyObject* new_py_dict = NULL;
  {
    PythonGilLock lock;
    new_py_dict = PyDict_Copy(py_dict);
  }

  return new DictLocalObject(new_py_dict);
}

string DictLocalObject::Dump() const {
  PyObject* const py_dict = py_object();

  string items_string = "{";
  bool item_found = false;

  {
    Py_ssize_t pos = 0;
    PyObject* py_key = NULL;
    PyObject* py_value = NULL;

    PythonGilLock lock;

    while (PyDict_Next(py_dict, &pos, &py_key, &py_value) != 0) {
      if (item_found) {
        items_string += ",";
      } else {
        item_found = true;
      }

      PeerObject* const key_peer_object = PyProxyObject_GetPeerObject(py_key);
      PeerObject* const value_peer_object = PyProxyObject_GetPeerObject(
          py_value);

      StringAppendF(&items_string, " %s: %s", key_peer_object->Dump().c_str(),
                    value_peer_object->Dump().c_str());
    }
  }

  if (item_found) {
    items_string += " }";
  } else {
    items_string = "{}";
  }

  return StringPrintf("{ \"type\": \"DictLocalObject\", \"items\": %s }",
                      items_string.c_str());
}

// static
DictLocalObject* DictLocalObject::ParseDictProto(
    const MappingProto& dict_proto, DeserializationContext* context) {
  CHECK(context != NULL);

  const int item_count = dict_proto.item_size();

  PyObject* py_dict = NULL;
  {
    PythonGilLock lock;

    py_dict = PyDict_New();
    CHECK(py_dict != NULL);

    for (int i = 0; i < item_count; ++i) {
      const MappingItemProto& item_proto = dict_proto.item(i);

      const int key_object_index = static_cast<int>(
          item_proto.key().object_index());
      const int value_object_index = static_cast<int>(
          item_proto.value().object_index());

      PeerObject* const key_peer_object = context->GetPeerObjectByIndex(
          key_object_index);
      PeerObject* const value_peer_object = context->GetPeerObjectByIndex(
          value_object_index);

      PyObject* const py_key = PyProxyObject_New(key_peer_object);
      PyObject* const py_value = PyProxyObject_New(value_peer_object);

      CHECK_EQ(PyDict_SetItem(py_dict, py_key, py_value), 0);
    }
  }

  return new DictLocalObject(py_dict);
}

void DictLocalObject::PopulateObjectProto(ObjectProto* object_proto,
                                          SerializationContext* context) const {
  CHECK(object_proto != NULL);
  CHECK(context != NULL);

  PyObject* const py_dict = py_object();
  MappingProto* const dict_proto = object_proto->mutable_dict_object();

  {
    Py_ssize_t pos = 0;
    PyObject* py_key = NULL;
    PyObject* py_value = NULL;

    PythonGilLock lock;

    while (PyDict_Next(py_dict, &pos, &py_key, &py_value) != 0) {
      PeerObject* const key_peer_object = PyProxyObject_GetPeerObject(py_key);
      PeerObject* const value_peer_object = PyProxyObject_GetPeerObject(
          py_value);

      const int key_object_index = context->GetIndexForPeerObject(
          key_peer_object);
      const int value_object_index = context->GetIndexForPeerObject(
          value_peer_object);

      MappingItemProto* const item = dict_proto->add_item();
      item->mutable_key()->set_object_index(key_object_index);
      item->mutable_value()->set_object_index(value_object_index);
    }
  }
}

bool DictLocalObject::InvokeTypeSpecificMethod(PeerObject* peer_object,
                                               const string& method_name,
                                               const vector<Value>& parameters,
                                               MethodContext* method_context,
                                               Value* return_value) {
  // TODO(dss): Implement this.
  return false;
}

}  // namespace python
}  // namespace floating_temple
