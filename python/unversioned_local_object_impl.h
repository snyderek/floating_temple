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

#ifndef PYTHON_UNVERSIONED_LOCAL_OBJECT_IMPL_H_
#define PYTHON_UNVERSIONED_LOCAL_OBJECT_IMPL_H_

#include "third_party/Python-3.4.2/Include/Python.h"

#include "include/c++/unversioned_local_object.h"

namespace floating_temple {
namespace python {

// Abstract class
// TODO(dss): Rename this class. The "Impl" suffix doesn't make sense for an
// abstract class.
class UnversionedLocalObjectImpl : public UnversionedLocalObject {
 public:
  // Steals a reference to 'py_object'.
  explicit UnversionedLocalObjectImpl(PyObject* py_object);
  ~UnversionedLocalObjectImpl() override;

  void InvokeMethod(Thread* thread,
                    PeerObject* peer_object,
                    const std::string& method_name,
                    const std::vector<Value>& parameters,
                    Value* return_value) override;

 protected:
  PyObject* py_object() const { return py_object_; }

 private:
  PyObject* const py_object_;
};

}  // namespace python
}  // namespace floating_temple

#endif  // PYTHON_UNVERSIONED_LOCAL_OBJECT_IMPL_H_
