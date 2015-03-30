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

#ifndef PYTHON_WRAP_OBJECT_H_
#define PYTHON_WRAP_OBJECT_H_

#include "third_party/Python-3.4.2/Include/Python.h"

#include "base/logging.h"
#include "include/c++/thread.h"
#include "python/interpreter_impl.h"
#include "python/py_proxy_object.h"

namespace floating_temple {

class LocalObject;
class PeerObject;

namespace python {

template<class LocalObjectType>
PeerObject* WrapPythonObject(PyObject* py_object) {
  CHECK(py_object != nullptr);
  CHECK(Py_TYPE(py_object) == &PyProxyObject_Type);

  InterpreterImpl* const interpreter = InterpreterImpl::instance();
  Thread* const thread = interpreter->GetThreadObject();

  LocalObject* const local_object = new LocalObjectType(py_object);
  return thread->CreatePeerObject(local_object);
}

}  // namespace python
}  // namespace floating_temple

#endif  // PYTHON_WRAP_OBJECT_H_
