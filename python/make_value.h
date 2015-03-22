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

#ifndef PYTHON_MAKE_VALUE_H_
#define PYTHON_MAKE_VALUE_H_

#include "third_party/Python-3.4.2/Include/Python.h"

#include "base/logging.h"

namespace floating_temple {

class Value;

namespace python {

class MethodContext;

void MakeValue(int in, Value* out);
void MakeValue(long in, Value* out);
void MakeValue(const char* in, Value* out);
void MakeValue(PyObject* in, Value* out);

template<typename T> T ExtractValue(const Value& value,
                                    MethodContext* method_context);
template<> int ExtractValue(const Value& value, MethodContext* method_context);
template<> long ExtractValue(const Value& value, MethodContext* method_context);
template<> char* ExtractValue(const Value& value,
                              MethodContext* method_context);
template<> PyObject* ExtractValue(const Value& value,
                                  MethodContext* method_context);

template<typename T> T GetExceptionReturnCode();
template<> inline int GetExceptionReturnCode() { return -1; }
template<> inline long GetExceptionReturnCode() { return -1; }
template<> inline PyObject* GetExceptionReturnCode() { return nullptr; }

template<typename T>
void MakeReturnValue(T in, Value* out) {
  if (in == GetExceptionReturnCode<T>()) {
    // TODO(dss): If the Python function call throws an exception, propagate it
    // to the caller instead of crashing.
    if (PyErr_Occurred() != nullptr) {
      PyErr_Print();
      LOG(FATAL) << "Python exception";
    }
  }

  MakeValue(in, out);
}

}  // namespace python
}  // namespace floating_temple

#endif  // PYTHON_MAKE_VALUE_H_
