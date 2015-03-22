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

#include "python/python_scoped_ptr.h"

#include "third_party/Python-3.4.2/Include/Python.h"

#include <algorithm>

#include "base/logging.h"

namespace floating_temple {
namespace python {

PythonScopedPtr::PythonScopedPtr(PyObject* object)
    : object_(object) {
}

PythonScopedPtr::~PythonScopedPtr() {
  Py_XDECREF(object_);
}

void PythonScopedPtr::reset(PyObject* object) {
  if (object != object_) {
    Py_XDECREF(object_);
    object_ = object;
  }
}

void PythonScopedPtr::IncRef() const {
  CHECK(object_ != nullptr);
  Py_INCREF(object_);
}

PyObject* PythonScopedPtr::release() {
  PyObject* const object = object_;
  object_ = nullptr;
  return object;
}

void PythonScopedPtr::swap(PythonScopedPtr& other) {
  std::swap(object_, other.object_);
}

}  // namespace python
}  // namespace floating_temple
