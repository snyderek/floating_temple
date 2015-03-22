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

#include "python/py_proxy_object.h"

#include "third_party/Python-3.4.2/Include/Python.h"

#include <string>
#include <vector>

#include "base/integral_types.h"
#include "base/logging.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "python/interpreter_impl.h"
#include "python/make_value.h"

using std::string;
using std::vector;

namespace floating_temple {

class PeerObject;

namespace python {
namespace {

const uint64 kMagicNumber = 0x32418b0f5ce3c367;

struct PyProxyObject {
  PyObject_HEAD
  uint64 magic_number;
  PeerObject* peer_object;
};

vector<Value> CreateParameterVector(PyObject* self) {
  return vector<Value>();
}

template<typename T2>
vector<Value> CreateParameterVector(PyObject* self, T2 arg2) {
  vector<Value> v(1);
  MakeValue(arg2, &v[0]);
  return v;
}

template<typename T2, typename T3>
vector<Value> CreateParameterVector(PyObject* self, T2 arg2, T3 arg3) {
  vector<Value> v(2);
  MakeValue(arg2, &v[0]);
  MakeValue(arg3, &v[1]);
  return v;
}

template<typename T>
T MethodBody(PyObject* self, const string& method_name,
             const vector<Value>& params) {
  VLOG(3) << "Shim method: " << method_name;

  InterpreterImpl* const interpreter = InterpreterImpl::instance();
  Thread* const thread = interpreter->GetThreadObject();

  PeerObject* const peer_object = PyProxyObject_GetPeerObject(self);

  Value return_value;
  bool success = false;
  {
    Py_BEGIN_ALLOW_THREADS
    success = thread->CallMethod(peer_object, method_name, params,
                                 &return_value);
    Py_END_ALLOW_THREADS
  }

  if (!success) {
    return GetExceptionReturnCode<T>();
  }

  return ExtractValue<T>(return_value, nullptr);
}

void shim_tp_dealloc(PyObject* op) {
  Py_TYPE(op)->tp_free(op);
}

#define PROXY_METHOD(method_name, return_type, function_name, params, args) \
    return_type function_name params { \
      return MethodBody<return_type>(self, method_name, \
                                     CreateParameterVector args); \
    }

PROXY_METHOD("tp_getattr", PyObject*, shim_tp_getattr,
             (PyObject* self, char* name), (self, name))
PROXY_METHOD("tp_setattr", int, shim_tp_setattr,
             (PyObject* self, char* name, PyObject* value), (self, name, value))
PROXY_METHOD("tp_repr", PyObject*, shim_tp_repr, (PyObject* self), (self))
PROXY_METHOD("tp_hash", long, shim_tp_hash, (PyObject* self), (self))
PROXY_METHOD("tp_call", PyObject*, shim_tp_call,
             (PyObject* self, PyObject* args, PyObject* kwds),
             (self, args, kwds))
PROXY_METHOD("tp_str", PyObject*, shim_tp_str, (PyObject* self), (self))
PROXY_METHOD("tp_getattro", PyObject*, shim_tp_getattro,
             (PyObject* self, PyObject* name), (self, name))
PROXY_METHOD("tp_setattro", int, shim_tp_setattro,
             (PyObject* self, PyObject* name, PyObject* value),
             (self, name, value))
PROXY_METHOD("tp_richcompare", PyObject*, shim_tp_richcompare,
             (PyObject* self, PyObject* other, int op), (self, other, op))
PROXY_METHOD("tp_iter", PyObject*, shim_tp_iter, (PyObject* self), (self))
PROXY_METHOD("tp_iternext", PyObject*, shim_tp_iternext, (PyObject* self),
             (self))
PROXY_METHOD("tp_descr_get", PyObject*, shim_tp_descr_get,
             (PyObject* self, PyObject* obj, PyObject* type), (self, obj, type))
PROXY_METHOD("tp_descr_set", int, shim_tp_descr_set,
             (PyObject* self, PyObject* obj, PyObject* value),
             (self, obj, value))
PROXY_METHOD("tp_init", int, shim_tp_init,
             (PyObject* self, PyObject* args, PyObject* kwds),
             (self, args, kwds))

PROXY_METHOD("nb_add", PyObject*, shim_nb_add, (PyObject* self, PyObject* b),
             (self, b))
PROXY_METHOD("nb_subtract", PyObject*, shim_nb_subtract,
             (PyObject* self, PyObject* b), (self, b))
PROXY_METHOD("nb_multiply", PyObject*, shim_nb_multiply,
             (PyObject* self, PyObject* b), (self, b))
PROXY_METHOD("nb_remainder", PyObject*, shim_nb_remainder,
             (PyObject* self, PyObject* b), (self, b))
PROXY_METHOD("nb_divmod", PyObject*, shim_nb_divmod,
             (PyObject* self, PyObject* b), (self, b))
PROXY_METHOD("nb_power", PyObject*, shim_nb_power,
             (PyObject* self, PyObject* b, PyObject* c), (self, b, c))
PROXY_METHOD("nb_negative", PyObject*, shim_nb_negative, (PyObject* self),
             (self))
PROXY_METHOD("nb_positive", PyObject*, shim_nb_positive, (PyObject* self),
             (self))
PROXY_METHOD("nb_absolute", PyObject*, shim_nb_absolute, (PyObject* self),
             (self))
PROXY_METHOD("nb_bool", int, shim_nb_bool, (PyObject* self), (self))
PROXY_METHOD("nb_invert", PyObject*, shim_nb_invert, (PyObject* self), (self))
PROXY_METHOD("nb_lshift", PyObject*, shim_nb_lshift,
             (PyObject* self, PyObject* b), (self, b))
PROXY_METHOD("nb_rshift", PyObject*, shim_nb_rshift,
             (PyObject* self, PyObject* b), (self, b))
PROXY_METHOD("nb_and", PyObject*, shim_nb_and, (PyObject* self, PyObject* b),
             (self, b))
PROXY_METHOD("nb_xor", PyObject*, shim_nb_xor, (PyObject* self, PyObject* b),
             (self, b))
PROXY_METHOD("nb_or", PyObject*, shim_nb_or, (PyObject* self, PyObject* b),
             (self, b))
PROXY_METHOD("nb_int", PyObject*, shim_nb_int, (PyObject* self), (self))
PROXY_METHOD("nb_float", PyObject*, shim_nb_float, (PyObject* self), (self))
PROXY_METHOD("nb_inplace_add", PyObject*, shim_nb_inplace_add,
             (PyObject* self, PyObject* b), (self, b))
PROXY_METHOD("nb_inplace_subtract", PyObject*, shim_nb_inplace_subtract,
             (PyObject* self, PyObject* b), (self, b))
PROXY_METHOD("nb_inplace_multiply", PyObject*, shim_nb_inplace_multiply,
             (PyObject* self, PyObject* b), (self, b))
PROXY_METHOD("nb_inplace_remainder", PyObject*, shim_nb_inplace_remainder,
             (PyObject* self, PyObject* b), (self, b))
PROXY_METHOD("nb_inplace_power", PyObject*, shim_nb_inplace_power,
             (PyObject* self, PyObject* b, PyObject* c), (self, b, c))
PROXY_METHOD("nb_inplace_lshift", PyObject*, shim_nb_inplace_lshift,
             (PyObject* self, PyObject* b), (self, b))
PROXY_METHOD("nb_inplace_rshift", PyObject*, shim_nb_inplace_rshift,
             (PyObject* self, PyObject* b), (self, b))
PROXY_METHOD("nb_inplace_and", PyObject*, shim_nb_inplace_and,
             (PyObject* self, PyObject* b), (self, b))
PROXY_METHOD("nb_inplace_xor", PyObject*, shim_nb_inplace_xor,
             (PyObject* self, PyObject* b), (self, b))
PROXY_METHOD("nb_inplace_or", PyObject*, shim_nb_inplace_or,
             (PyObject* self, PyObject* b), (self, b))
PROXY_METHOD("nb_floor_divide", PyObject*, shim_nb_floor_divide,
             (PyObject* self, PyObject* b), (self, b))
PROXY_METHOD("nb_true_divide", PyObject*, shim_nb_true_divide,
             (PyObject* self, PyObject* b), (self, b))
PROXY_METHOD("nb_inplace_floor_divide", PyObject*, shim_nb_inplace_floor_divide,
             (PyObject* self, PyObject* b), (self, b))
PROXY_METHOD("nb_inplace_true_divide", PyObject*, shim_nb_inplace_true_divide,
             (PyObject* self, PyObject* b), (self, b))
PROXY_METHOD("nb_index", PyObject*, shim_nb_index, (PyObject* self), (self))

PROXY_METHOD("sq_length", Py_ssize_t, shim_sq_length, (PyObject* self), (self))
PROXY_METHOD("sq_concat", PyObject*, shim_sq_concat,
             (PyObject* self, PyObject* b), (self, b))
PROXY_METHOD("sq_repeat", PyObject*, shim_sq_repeat,
             (PyObject* self, Py_ssize_t b), (self, b))
PROXY_METHOD("sq_item", PyObject*, shim_sq_item, (PyObject* self, Py_ssize_t b),
             (self, b))
PROXY_METHOD("sq_ass_item", int, shim_sq_ass_item,
             (PyObject* self, Py_ssize_t b, PyObject* c), (self, b, c))
PROXY_METHOD("sq_contains", int, shim_sq_contains,
             (PyObject* self, PyObject* b), (self, b))
PROXY_METHOD("sq_inplace_concat", PyObject*, shim_sq_inplace_concat,
             (PyObject* self, PyObject* b), (self, b))
PROXY_METHOD("sq_inplace_repeat", PyObject*, shim_sq_inplace_repeat,
             (PyObject* self, Py_ssize_t b), (self, b))

PROXY_METHOD("mp_length", Py_ssize_t, shim_mp_length, (PyObject* self), (self))
PROXY_METHOD("mp_subscript", PyObject*, shim_mp_subscript,
             (PyObject* self, PyObject* b), (self, b))
PROXY_METHOD("mp_ass_subscript", int, shim_mp_ass_subscript,
             (PyObject* self, PyObject* b, PyObject* c), (self, b, c))

#undef PROXY_METHOD

// TODO(dss): Add entries for all of the struct members.

PyNumberMethods proxy_as_number = {
  shim_nb_add,                                // nb_add
  shim_nb_subtract,                           // nb_subtract
  shim_nb_multiply,                           // nb_multiply
  shim_nb_remainder,                          // nb_remainder
  shim_nb_divmod,                             // nb_divmod
  shim_nb_power,                              // nb_power
  shim_nb_negative,                           // nb_negative
  shim_nb_positive,                           // nb_positive
  shim_nb_absolute,                           // nb_absolute
  shim_nb_bool,                               // nb_bool
  shim_nb_invert,                             // nb_invert
  shim_nb_lshift,                             // nb_lshift
  shim_nb_rshift,                             // nb_rshift
  shim_nb_and,                                // nb_and
  shim_nb_xor,                                // nb_xor
  shim_nb_or,                                 // nb_or
  shim_nb_int,                                // nb_int
  nullptr,                                    // nb_reserved
  shim_nb_float,                              // nb_float
  shim_nb_inplace_add,                        // nb_inplace_add
  shim_nb_inplace_subtract,                   // nb_inplace_subtract
  shim_nb_inplace_multiply,                   // nb_inplace_multiply
  shim_nb_inplace_remainder,                  // nb_inplace_remainder
  shim_nb_inplace_power,                      // nb_inplace_power
  shim_nb_inplace_lshift,                     // nb_inplace_lshift
  shim_nb_inplace_rshift,                     // nb_inplace_rshift
  shim_nb_inplace_and,                        // nb_inplace_and
  shim_nb_inplace_xor,                        // nb_inplace_xor
  shim_nb_inplace_or,                         // nb_inplace_or
  shim_nb_floor_divide,                       // nb_floor_divide
  shim_nb_true_divide,                        // nb_true_divide
  shim_nb_inplace_floor_divide,               // nb_inplace_floor_divide
  shim_nb_inplace_true_divide,                // nb_inplace_true_divide
  shim_nb_index                               // nb_index
};

PySequenceMethods proxy_as_sequence = {
  shim_sq_length,                             // sq_length
  shim_sq_concat,                             // sq_concat
  shim_sq_repeat,                             // sq_repeat
  shim_sq_item,                               // sq_item
  nullptr,                                    // sq_slice (deprecated)
  shim_sq_ass_item,                           // sq_ass_item
  nullptr,                                    // sq_ass_slice (deprecated)
  shim_sq_contains,                           // sq_contains
  shim_sq_inplace_concat,                     // sq_inplace_concat
  shim_sq_inplace_repeat                      // sq_inplace_repeat
};

PyMappingMethods proxy_as_mapping = {
  shim_mp_length,                             // mp_length
  shim_mp_subscript,                          // mp_subscript
  shim_mp_ass_subscript                       // mp_ass_subscript
};

}  // namespace

// TODO(dss): Figure out how to handle class methods like list.from_bytes.

PyTypeObject PyProxyObject_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "floating_temple_proxy",                    // tp_name
  sizeof (PyProxyObject),                     // tp_basicsize
  0,                                          // tp_itemsize
  shim_tp_dealloc,                            // tp_dealloc
  nullptr,                                    // tp_print
  shim_tp_getattr,                            // tp_getattr
  shim_tp_setattr,                            // tp_setattr
  nullptr,                                    // tp_reserved
  shim_tp_repr,                               // tp_repr
  &proxy_as_number,                           // tp_as_number
  &proxy_as_sequence,                         // tp_as_sequence
  &proxy_as_mapping,                          // tp_as_mapping
  shim_tp_hash,                               // tp_hash
  shim_tp_call,                               // tp_call
  shim_tp_str,                                // tp_str
  shim_tp_getattro,                           // tp_getattro
  shim_tp_setattro,                           // tp_setattro
  nullptr,                                    // tp_as_buffer
  // TODO(dss): Include any other flags that might be needed.
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   // tp_flags
  "TODO(dss): Write this docstring",          // tp_doc
  nullptr,                                    // tp_traverse
  nullptr,                                    // tp_clear
  shim_tp_richcompare,                        // tp_richcompare
  0,                                          // tp_weaklistoffset
  shim_tp_iter,                               // tp_iter
  shim_tp_iternext,                           // tp_iternext
  nullptr,                                    // tp_methods
  nullptr,                                    // tp_members
  nullptr,                                    // tp_getset
  nullptr,                                    // tp_base
  nullptr,                                    // tp_dict
  shim_tp_descr_get,                          // tp_descr_get
  shim_tp_descr_set,                          // tp_descr_set
  0,                                          // tp_dictoffset
  shim_tp_init,                               // tp_init
  PyType_GenericAlloc,                        // tp_alloc
  PyType_GenericNew,                          // tp_new
  PyObject_Del                                // tp_free
};

PyObject* PyProxyObject_New(PeerObject* peer_object) {
  CHECK(peer_object != nullptr);

  PyProxyObject* const py_proxy_object = PyObject_New(PyProxyObject,
                                                      &PyProxyObject_Type);
  CHECK(py_proxy_object != nullptr);

  py_proxy_object->magic_number = kMagicNumber;
  py_proxy_object->peer_object = peer_object;

  return reinterpret_cast<PyObject*>(py_proxy_object);
}

PeerObject* PyProxyObject_GetPeerObject(PyObject* py_object) {
  CHECK(py_object != nullptr);

  PyProxyObject* const py_proxy_object = reinterpret_cast<PyProxyObject*>(
      py_object);

  CHECK_EQ(py_proxy_object->magic_number, kMagicNumber);
  CHECK(py_proxy_object->peer_object != nullptr);

  return py_proxy_object->peer_object;
}

}  // namespace python
}  // namespace floating_temple
