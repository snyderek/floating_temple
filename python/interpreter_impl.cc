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
#include <utility>
#include <vector>

#include "base/integral_types.h"
#include "base/logging.h"
#include "base/mutex.h"
#include "base/mutex_lock.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "python/local_object_impl.h"
#include "python/long_local_object.h"
#include "python/py_proxy_object.h"
#include "python/python_gil_lock.h"
#include "python/unserializable_local_object.h"

using std::make_pair;
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

bool InterpreterImpl::CallMethod(PeerObject* peer_object,
                                 const string& method_name,
                                 const vector<Value>& parameters,
                                 Value* return_value) {
  return GetThreadObject()->CallMethod(peer_object, method_name, parameters,
                                       return_value);
}

Thread* InterpreterImpl::SetThreadObject(Thread* new_thread) {
  Thread* const old_thread = thread_object_;
  thread_object_ = new_thread;
  return old_thread;
}

LocalObject* InterpreterImpl::DeserializeObject(
    const void* buffer, size_t buffer_size, DeserializationContext* context) {
  return LocalObjectImpl::Deserialize(buffer, buffer_size, context);
}

PyObject* InterpreterImpl::PeerObjectToPyProxyObject(PeerObject* peer_object) {
  CHECK(peer_object != nullptr);

  PyObject* py_new_proxy_object = nullptr;
  PyObject* py_existing_proxy_object = nullptr;

  {
    PythonGilLock lock;
    py_new_proxy_object = PyProxyObject_New(peer_object);
    CHECK(py_new_proxy_object != nullptr);
    Py_INCREF(py_new_proxy_object);
  }

  {
    MutexLock lock(&objects_mu_);

    const auto insert_result = proxy_objects_.insert(
        make_pair(peer_object, py_new_proxy_object));

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

PeerObject* InterpreterImpl::PyProxyObjectToPeerObject(PyObject* py_object) {
  CHECK(py_object != nullptr);

  const PyTypeObject* const py_type = Py_TYPE(py_object);
  if (py_type == &PyProxyObject_Type) {
    return PyProxyObject_GetPeerObject(py_object);
  }

  PeerObject* new_peer_object = nullptr;
  if (py_type == &PyLong_Type) {
    new_peer_object = CreateUnnamedPeerObject<LongLocalObject>(py_object);
  } else {
    new_peer_object = CreateUnnamedPeerObject<UnserializableLocalObject>(
        py_object);
  }

  PeerObject* existing_peer_object = nullptr;

  {
    MutexLock lock(&objects_mu_);

    const auto insert_result = unserializable_objects_.insert(
        make_pair(py_object, new_peer_object));

    if (insert_result.second) {
      CHECK(proxy_objects_.insert(make_pair(new_peer_object,
                                            py_object)).second);
      return new_peer_object;
    }
    existing_peer_object = insert_result.first->second;
  }

  // TODO(dss): Delete new_peer_object.

  return existing_peer_object;
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
