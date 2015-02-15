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

#include "python/run_python_program.h"

#include "third_party/Python-3.4.2/Include/Python.h"
#include "third_party/Python-3.4.2/Include/dss_hooks.h"

#include <cstddef>
#include <cstdio>
#include <string>

#include "base/logging.h"
#include "include/c++/peer.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "python/interpreter_impl.h"
#include "python/list_local_object.h"
#include "python/long_local_object.h"
#include "python/peer_module.h"
#include "python/program_object.h"
#include "python/py_proxy_object.h"
#include "python/python_gil_lock.h"
#include "python/python_scoped_ptr.h"

using std::FILE;
using std::fclose;
using std::fopen;
using std::string;

namespace floating_temple {

class PeerObject;

namespace python {
namespace {

template<class LocalObjectType>
PyObject* WrapPythonObject(PyObject* py_object) {
  if (py_object == NULL) {
    return NULL;
  }

  InterpreterImpl* const interpreter = InterpreterImpl::instance();
  Thread* const thread = interpreter->GetThreadObject();

  LocalObjectImpl* const local_object = new LocalObjectType(py_object);
  PeerObject* const peer_object = thread->CreatePeerObject(local_object);

  return PyProxyObject_New(peer_object);
}

PyObject* WrapPythonList(PyObject* py_list_object) {
  return WrapPythonObject<ListLocalObject>(py_list_object);
}

PyObject* WrapPythonLong(PyObject* py_long_object) {
  return WrapPythonObject<LongLocalObject>(py_long_object);
}

}  // namespace

void RunPythonProgram(Peer* peer, const string& source_file_name) {
  // Run the source file.
  FILE* const fp = fopen(source_file_name.c_str(), "r");
  PLOG_IF(FATAL, fp == NULL) << "fopen";

  RunPythonFile(peer, fp, source_file_name);

  PLOG_IF(FATAL, fclose(fp) != 0) << "fclose";
}

void RunPythonFile(Peer* peer, FILE* fp, const string& source_file_name) {
  CHECK(peer != NULL);

  CHECK_NE(PyImport_AppendInittab("peer", PyInit_peer), -1);

  // Calling Py_InitializeEx with a parameter of 0 causes signal handler
  // registration to be skipped.
  Py_InitializeEx(0);

  Py_InstallListCreationHook(WrapPythonList);
  Py_InstallLongCreationHook(WrapPythonLong);

  LocalObject* program_object = NULL;
  {
    PythonGilLock lock;

    // The following code is adapted from the PyRun_SimpleFileExFlags function
    // in "third_party/Python-3.4.2/Python/pythonrun.c".

    PyObject* const module = PyImport_AddModule("__main__");
    CHECK(module != NULL);

    PyObject* const globals = PyModule_GetDict(module);
    CHECK(globals != NULL);

    if (PyDict_GetItemString(globals, "__file__") == NULL) {
      const PythonScopedPtr py_file_name(
          PyUnicode_DecodeFSDefaultAndSize(source_file_name.data(),
                                           source_file_name.length()));
      CHECK(py_file_name.get() != NULL);

      CHECK_EQ(PyDict_SetItemString(globals, "__file__", py_file_name.get()),
               0);
      CHECK_EQ(PyDict_SetItemString(globals, "__cached__", Py_None), 0);
    }

    program_object = new ProgramObject(fp, source_file_name, globals);
  }

  Value return_value;
  peer->RunProgram(program_object, "run", &return_value);

  Py_Finalize();
}

}  // namespace python
}  // namespace floating_temple
