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
#include "base/string_printf.h"
#include "include/c++/deserialization_context.h"
#include "include/c++/peer_object.h"
#include "include/c++/serialization_context.h"
#include "include/c++/value.h"
#include "python/call_method.h"
#include "python/local_object_impl.h"
#include "python/make_value.h"
#include "python/proto/serialization.pb.h"
#include "python/py_proxy_object.h"
#include "python/python_gil_lock.h"

using std::string;
using std::vector;

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
        PeerObject* const peer_object = PyProxyObject_GetPeerObject(py_item);

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
      PyObject* const py_item = PyProxyObject_New(peer_object);
      CHECK_EQ(PyList_SetItem(py_list, static_cast<Py_ssize_t>(i), py_item), 0);
    }
  }

  return new ListLocalObject(py_list);
}

void ListLocalObject::PopulateObjectProto(ObjectProto* object_proto,
                                          SerializationContext* context) const {
  CHECK(object_proto != nullptr);
  CHECK(context != nullptr);

  PyObject* const py_list = py_object();
  SequenceProto* const list_proto = object_proto->mutable_list_object();

  {
    PythonGilLock lock;

    const Py_ssize_t length = PyList_Size(py_list);

    for (Py_ssize_t i = 0; i < length; ++i) {
      PyObject* const py_item = PyList_GetItem(py_list, i);
      PeerObject* const peer_object = PyProxyObject_GetPeerObject(py_item);
      const int object_index = context->GetIndexForPeerObject(peer_object);
      list_proto->add_item()->set_object_index(object_index);
    }
  }
}

#define CALL_FUNCTION0(function) \
    do { \
      if (method_name == #function) { \
        CHECK_EQ(parameters.size(), 0u); \
        MakeReturnValue(function(py_object()), return_value); \
        return true; \
      } \
    } while (false)

#define CALL_FUNCTION1(function, param_type1) \
    do { \
      if (method_name == #function) { \
        CHECK_EQ(parameters.size(), 1u); \
        MakeReturnValue( \
            function( \
                py_object(), \
                ExtractValue<param_type1>(parameters[0], method_context)), \
            return_value); \
        return true; \
      } \
    } while (false)

#define CALL_FUNCTION2(function, param_type1, param_type2) \
    do { \
      if (method_name == #function) { \
        CHECK_EQ(parameters.size(), 2u); \
        MakeReturnValue( \
            function( \
                py_object(), \
                ExtractValue<param_type1>(parameters[0], method_context), \
                ExtractValue<param_type2>(parameters[1], method_context)), \
            return_value); \
        return true; \
      } \
    } while (false)

#define CALL_FUNCTION3(function, param_type1, param_type2, param_type3) \
    do { \
      if (method_name == #function) { \
        CHECK_EQ(parameters.size(), 3u); \
        MakeReturnValue( \
            function( \
                py_object(), \
                ExtractValue<param_type1>(parameters[0], method_context), \
                ExtractValue<param_type2>(parameters[1], method_context), \
                ExtractValue<param_type3>(parameters[2], method_context)), \
            return_value); \
        return true; \
      } \
    } while (false)

#define CALL_NORMAL_METHOD(method) \
    do { \
      if (method_name == #method) { \
        CallNormalMethod(py_object(), method_name, parameters, return_value); \
        return true; \
      } \
    } while (false)

#define CALL_VARARGS_METHOD(method) \
    do { \
      if (method_name == #method) { \
        CallVarargsMethod(py_object(), method_name, parameters, return_value); \
        return true; \
      } \
    } while (false)

bool ListLocalObject::InvokeTypeSpecificMethod(PeerObject* peer_object,
                                               const string& method_name,
                                               const vector<Value>& parameters,
                                               MethodContext* method_context,
                                               Value* return_value) {
  // TODO(dss): Remove support for these methods.

  CALL_FUNCTION1(PyList_Append, PyObject*);
  CALL_FUNCTION0(PyList_AsTuple);
  CALL_FUNCTION1(PyList_GetItem, Py_ssize_t);
  CALL_FUNCTION2(PyList_GetSlice, Py_ssize_t, Py_ssize_t);
  CALL_FUNCTION2(PyList_Insert, Py_ssize_t, PyObject*);
  CALL_FUNCTION0(PyList_Reverse);
  CALL_FUNCTION2(PyList_SetItem, Py_ssize_t, PyObject*);
  CALL_FUNCTION3(PyList_SetSlice, Py_ssize_t, Py_ssize_t, PyObject*);
  CALL_FUNCTION0(PyList_Size);
  CALL_FUNCTION0(PyList_Sort);

  if (method_name == "_PyList_Extend") {
    CHECK_EQ(parameters.size(), 1u);
    MakeReturnValue(
        _PyList_Extend(
            reinterpret_cast<PyListObject*>(py_object()),
            ExtractValue<PyObject*>(parameters[0], method_context)),
        return_value);
    return true;
  }

  CALL_NORMAL_METHOD(__getitem__);
  CALL_NORMAL_METHOD(__reversed__);
  CALL_NORMAL_METHOD(__sizeof__);
  CALL_NORMAL_METHOD(clear);
  CALL_NORMAL_METHOD(copy);
  CALL_NORMAL_METHOD(append);
  CALL_VARARGS_METHOD(insert);
  CALL_NORMAL_METHOD(extend);
  CALL_VARARGS_METHOD(pop);
  CALL_NORMAL_METHOD(remove);
  CALL_VARARGS_METHOD(index);
  CALL_NORMAL_METHOD(count);
  CALL_NORMAL_METHOD(reverse);
  CALL_VARARGS_METHOD(sort);

  return false;
}

#undef CALL_FUNCTION0
#undef CALL_FUNCTION1
#undef CALL_FUNCTION2
#undef CALL_FUNCTION3
#undef CALL_NORMAL_METHOD
#undef CALL_VARARGS_METHOD

}  // namespace python
}  // namespace floating_temple
