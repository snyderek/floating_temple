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

#include "python/peer_module.h"

#include "third_party/Python-3.4.2/Include/Python.h"

#include "python/interpreter_impl.h"

namespace floating_temple {
namespace python {
namespace {

PyObject* peer_BeginTransaction(PyObject* self, PyObject* unused) {
  InterpreterImpl::instance()->BeginTransaction();
  Py_RETURN_NONE;
}

PyObject* peer_EndTransaction(PyObject* self, PyObject* unused) {
  InterpreterImpl::instance()->EndTransaction();
  Py_RETURN_NONE;
}

PyMethodDef g_module_methods[] = {
  { "BeginTransaction",
    peer_BeginTransaction,
    METH_NOARGS,
    "TODO(dss): Write this doc string."
  },
  { "EndTransaction",
    peer_EndTransaction,
    METH_NOARGS,
    "TODO(dss): Write this doc string."
  },
  { nullptr, nullptr, 0, nullptr }
};

PyModuleDef g_module_def = {
  PyModuleDef_HEAD_INIT,
  "peer",                               // m_name
  "TODO(dss): Write this doc string.",  // m_doc
  -1,                                   // m_size
  g_module_methods,                     // m_methods
  nullptr,                              // m_reload
  nullptr,                              // m_traverse
  nullptr,                              // m_clear
  nullptr                               // m_free
};

}  // namespace

PyMODINIT_FUNC PyInit_peer() {
  return PyModule_Create(&g_module_def);
}

}  // namespace python
}  // namespace floating_temple
