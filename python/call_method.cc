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

#include "python/call_method.h"

#include "third_party/Python-3.4.2/Include/Python.h"

#include <string>
#include <vector>

#include "base/logging.h"
#include "include/c++/value.h"
#include "python/make_value.h"
#include "python/python_scoped_ptr.h"

using std::string;
using std::vector;

namespace floating_temple {
namespace python {
namespace {

void CallMethod(PyObject* const py_object,
                const string& method_name,
                PyObject* args,
                PyObject* kw,
                Value* return_value) {
  CHECK(py_object != nullptr);
  CHECK(!method_name.empty());
  CHECK(args != nullptr);

  const PythonScopedPtr method(PyObject_GetAttrString(py_object,
                                                      method_name.c_str()));
  CHECK(method.get() != nullptr);
  CHECK(PyCallable_Check(method.get()));

  MakeReturnValue(PyObject_Call(method.get(), args, kw), return_value);
}

}  // namespace

void CallNormalMethod(PyObject* const py_object,
                      const string& method_name,
                      const vector<Value>& parameters,
                      Value* return_value) {
  const int parameter_count = static_cast<int>(parameters.size());

  const PythonScopedPtr args(
      PyTuple_New(static_cast<Py_ssize_t>(parameter_count)));
  CHECK(args.get() != nullptr);

  for (int i = 0; i < parameter_count; ++i) {
    PyObject* const parameter = ExtractValue<PyObject*>(parameters[i], nullptr);
    CHECK(parameter != nullptr);
    Py_INCREF(parameter);
    PyTuple_SET_ITEM(args.get(), i, parameter);
  }

  CallMethod(py_object, method_name, args.get(), nullptr, return_value);
}

void CallVarargsMethod(PyObject* const py_object,
                       const string& method_name,
                       const vector<Value>& parameters,
                       Value* return_value) {
  const int parameter_count = static_cast<int>(parameters.size());

  CHECK_GE(parameter_count, 1);
  CHECK_LE(parameter_count, 2);

  PyObject* const args = ExtractValue<PyObject*>(parameters[0], nullptr);

  PyObject* kw = nullptr;
  if (parameter_count == 2) {
    kw = ExtractValue<PyObject*>(parameters[1], nullptr);
  }

  CallMethod(py_object, method_name, args, kw, return_value);
}

}  // namespace python
}  // namespace floating_temple
