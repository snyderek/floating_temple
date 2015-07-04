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

#ifndef PYTHON_INTERPRETER_IMPL_H_
#define PYTHON_INTERPRETER_IMPL_H_

#include "third_party/Python-3.4.2/Include/Python.h"

#include <string>
#include <unordered_map>
#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "base/mutex.h"
#include "include/c++/interpreter.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "python/py_proxy_object.h"
#include "python/python_gil_lock.h"

namespace floating_temple {

class ObjectReference;
class Thread;
class VersionedLocalObject;

namespace python {

class InterpreterImpl : public Interpreter {
 public:
  InterpreterImpl();
  ~InterpreterImpl() override;

  void BeginTransaction();
  void EndTransaction();
  bool CallMethod(ObjectReference* object_reference,
                  const std::string& method_name,
                  const std::vector<Value>& parameters,
                  Value* return_value);

  template<class LocalObjectType> ObjectReference* CreateVersionedObject(
      PyObject* py_object, const std::string& name);
  template<class LocalObjectType> ObjectReference* CreateUnversionedObject(
      PyObject* py_object, const std::string& name);

  Thread* SetThreadObject(Thread* new_thread);

  VersionedLocalObject* DeserializeObject(
      const void* buffer, std::size_t buffer_size,
      DeserializationContext* context) override;

  PyObject* ObjectReferenceToPyProxyObject(ObjectReference* object_reference);
  ObjectReference* PyProxyObjectToObjectReference(PyObject* py_object);

  static InterpreterImpl* instance();

 private:
  Thread* GetThreadObject();

  std::unordered_map<ObjectReference*, PyObject*> proxy_objects_;
  std::unordered_map<PyObject*, ObjectReference*> unserializable_objects_;
  mutable Mutex objects_mu_;

  static __thread Thread* thread_object_;
  static InterpreterImpl* instance_;

  DISALLOW_COPY_AND_ASSIGN(InterpreterImpl);
};

template<class LocalObjectType>
ObjectReference* InterpreterImpl::CreateVersionedObject(
    PyObject* py_object, const std::string& name) {
  CHECK(py_object != nullptr);
  CHECK(Py_TYPE(py_object) != &PyProxyObject_Type);

  {
    PythonGilLock lock;
    Py_INCREF(py_object);
  }

  LocalObjectType* const local_object = new LocalObjectType(py_object);
  return GetThreadObject()->CreateVersionedObject(local_object, name);
}

template<class LocalObjectType>
ObjectReference* InterpreterImpl::CreateUnversionedObject(
    PyObject* py_object, const std::string& name) {
  CHECK(py_object != nullptr);
  CHECK(Py_TYPE(py_object) != &PyProxyObject_Type);

  {
    PythonGilLock lock;
    Py_INCREF(py_object);
  }

  LocalObjectType* const local_object = new LocalObjectType(py_object);
  return GetThreadObject()->CreateUnversionedObject(local_object, name);
}

}  // namespace python
}  // namespace floating_temple

#endif  // PYTHON_INTERPRETER_IMPL_H_
