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

#ifndef PYTHON_PYTHON_SCOPED_PTR_H_
#define PYTHON_PYTHON_SCOPED_PTR_H_

#include "third_party/Python-3.4.2/Include/Python.h"

#include "base/logging.h"
#include "base/macros.h"

namespace floating_temple {
namespace python {

class PythonScopedPtr {
 public:
  explicit PythonScopedPtr(PyObject* object = nullptr);
  ~PythonScopedPtr();

  void reset(PyObject* object);

  PyObject& operator*() const;
  PyObject* operator->() const;
  PyObject* get() const;

  void IncRef() const;
  PyObject* release();
  void swap(PythonScopedPtr& other);

 private:
  PyObject* GetObject() const;

  PyObject* object_;

  DISALLOW_COPY_AND_ASSIGN(PythonScopedPtr);
};

inline PyObject* PythonScopedPtr::GetObject() const {
  CHECK(object_ != nullptr);
  return object_;
}

inline PyObject& PythonScopedPtr::operator*() const {
  return *GetObject();
}

inline PyObject* PythonScopedPtr::operator->() const {
  return GetObject();
}

inline PyObject* PythonScopedPtr::get() const {
  return object_;
}

}  // namespace python
}  // namespace floating_temple

#endif  // PYTHON_PYTHON_SCOPED_PTR_H_
