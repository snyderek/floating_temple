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

#include <cstdio>
#include <string>

#include "base/logging.h"
#include "include/c++/peer.h"
#include "include/c++/value.h"
#include "python/program_object.h"
#include "python/python_scoped_ptr.h"

using std::FILE;
using std::fclose;
using std::fopen;
using std::string;

namespace floating_temple {

class VersionedLocalObject;

namespace python {

void RunPythonProgram(Peer* peer, const string& source_file_name, bool linger) {
  // Run the source file.
  FILE* const fp = fopen(source_file_name.c_str(), "r");
  PLOG_IF(FATAL, fp == nullptr) << "fopen";

  RunPythonFile(peer, fp, source_file_name, linger);

  PLOG_IF(FATAL, fclose(fp) != 0) << "fclose";
}

void RunPythonFile(Peer* peer,
                   FILE* fp,
                   const string& source_file_name,
                   bool linger) {
  CHECK(peer != nullptr);

  // The following code is adapted from the PyRun_SimpleFileExFlags function in
  // "third_party/Python-3.4.2/Python/pythonrun.c".

  PyObject* const module = PyImport_AddModule("__main__");
  CHECK(module != nullptr);

  PyObject* const globals = PyModule_GetDict(module);
  CHECK(globals != nullptr);

  if (PyDict_GetItemString(globals, "__file__") == nullptr) {
    const PythonScopedPtr py_file_name(
        PyUnicode_DecodeFSDefaultAndSize(source_file_name.data(),
                                         source_file_name.length()));
    CHECK(py_file_name.get() != nullptr);

    CHECK_EQ(PyDict_SetItemString(globals, "__file__", py_file_name.get()), 0);
    CHECK_EQ(PyDict_SetItemString(globals, "__cached__", Py_None), 0);
  }

  VersionedLocalObject* const program_object = new ProgramObject(
      fp, source_file_name, globals);

  Py_BEGIN_ALLOW_THREADS
  Value return_value;
  peer->RunProgram(program_object, "run", &return_value, linger);
  Py_END_ALLOW_THREADS
}

}  // namespace python
}  // namespace floating_temple
