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

#include "python/interpreter_impl.h"

#include "third_party/Python-3.4.2/Include/Python.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/integral_types.h"
#include "base/logging.h"
#include "base/mutex.h"
#include "base/mutex_lock.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "python/long_local_object.h"
#include "python/py_proxy_object.h"
#include "python/python_gil_lock.h"
#include "python/unicode_local_object.h"
#include "python/unserializable_local_object.h"
#include "python/versioned_local_object_impl.h"

using std::size_t;
using std::string;
using std::vector;

namespace floating_temple {
namespace python {

__thread Thread* InterpreterImpl::thread_object_ = nullptr;
InterpreterImpl* InterpreterImpl::instance_ = nullptr;

InterpreterImpl::InterpreterImpl() {
  CHECK(instance_ == nullptr);
  instance_ = this;
}

InterpreterImpl::~InterpreterImpl() {
  instance_ = nullptr;
}

void InterpreterImpl::BeginTransaction() {
  GetThreadObject()->BeginTransaction();
}

void InterpreterImpl::EndTransaction() {
  GetThreadObject()->EndTransaction();
}

bool InterpreterImpl::CallMethod(ObjectReference* object_reference,
                                 const string& method_name,
                                 const vector<Value>& parameters,
                                 Value* return_value) {
  return GetThreadObject()->CallMethod(object_reference, method_name,
                                       parameters, return_value);
}

Thread* InterpreterImpl::SetThreadObject(Thread* new_thread) {
  Thread* const old_thread = thread_object_;
  thread_object_ = new_thread;
  return old_thread;
}

VersionedLocalObject* InterpreterImpl::DeserializeObject(
    const void* buffer, size_t buffer_size, DeserializationContext* context) {
  return VersionedLocalObjectImpl::Deserialize(buffer, buffer_size, context);
}

PyObject* InterpreterImpl::ObjectReferenceToPyProxyObject(
    ObjectReference* object_reference) {
  CHECK(object_reference != nullptr);

  PyObject* py_new_proxy_object = nullptr;
  PyObject* py_existing_proxy_object = nullptr;

  {
    PythonGilLock lock;
    py_new_proxy_object = PyProxyObject_New(object_reference);
    CHECK(py_new_proxy_object != nullptr);
    Py_INCREF(py_new_proxy_object);
  }

  {
    MutexLock lock(&objects_mu_);

    const auto insert_result = proxy_objects_.emplace(object_reference,
                                                      py_new_proxy_object);

    if (insert_result.second) {
      return py_new_proxy_object;
    }
    py_existing_proxy_object = insert_result.first->second;
  }

  {
    PythonGilLock lock;
    Py_DECREF(py_new_proxy_object);
    Py_DECREF(py_new_proxy_object);
  }

  return py_existing_proxy_object;
}

ObjectReference* InterpreterImpl::PyProxyObjectToObjectReference(
    PyObject* py_object) {
  CHECK(py_object != nullptr);

  const PyTypeObject* const py_type = Py_TYPE(py_object);
  if (py_type == &PyProxyObject_Type) {
    return PyProxyObject_GetObjectReference(py_object);
  }

  ObjectReference* new_object_reference = nullptr;
  if (py_type == &PyLong_Type) {
    new_object_reference = CreateVersionedObject<LongLocalObject>(py_object,
                                                                  "");
  } else if (py_type == &PyUnicode_Type) {
    new_object_reference = CreateVersionedObject<UnicodeLocalObject>(py_object,
                                                                    "");
  } else {
    new_object_reference = CreateUnversionedObject<UnserializableLocalObject>(
        py_object, "");
  }

  ObjectReference* existing_object_reference = nullptr;

  {
    MutexLock lock(&objects_mu_);

    const auto insert_result = unserializable_objects_.emplace(
        py_object, new_object_reference);

    if (insert_result.second) {
      CHECK(proxy_objects_.emplace(new_object_reference, py_object).second);
      return new_object_reference;
    }
    existing_object_reference = insert_result.first->second;
  }

  // TODO(dss): [BUG] Delete new_object_reference.

  return existing_object_reference;
}

// static
InterpreterImpl* InterpreterImpl::instance() {
  CHECK(instance_ != nullptr);
  return instance_;
}

Thread* InterpreterImpl::GetThreadObject() {
  CHECK(thread_object_ != nullptr);
  return thread_object_;
}

}  // namespace python
}  // namespace floating_temple
