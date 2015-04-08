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

#include "python/program_object.h"

#include "third_party/Python-3.4.2/Include/Python.h"
#include "third_party/Python-3.4.2/Include/floating_temple_hooks.h"

#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

#include "base/integral_types.h"
#include "base/logging.h"
#include "include/c++/thread.h"
#include "python/dict_local_object.h"
#include "python/false_local_object.h"
#include "python/interpreter_impl.h"
#include "python/list_local_object.h"
#include "python/long_local_object.h"
#include "python/none_local_object.h"
#include "python/proto/local_type.pb.h"
#include "python/proto/serialization.pb.h"
#include "python/python_gil_lock.h"
#include "python/python_scoped_ptr.h"
#include "python/thread_substitution.h"
#include "python/true_local_object.h"

using std::FILE;
using std::rewind;
using std::size_t;
using std::string;
using std::vector;

namespace floating_temple {

class PeerObject;

namespace python {
namespace {

template<class LocalObjectType>
PyObject* WrapPythonObject(PyObject* py_object) {
  if (py_object == nullptr) {
    return nullptr;
  }

  InterpreterImpl* const interpreter = InterpreterImpl::instance();
  PeerObject* const peer_object =
      interpreter->CreateUnnamedPeerObject<LocalObjectType>(py_object);
  return interpreter->PeerObjectToPyProxyObject(peer_object);
}

PyObject* WrapPythonList(PyObject* py_list_object) {
  return WrapPythonObject<ListLocalObject>(py_list_object);
}

PyObject* WrapPythonLong(PyObject* py_long_object) {
  return WrapPythonObject<LongLocalObject>(py_long_object);
}

}  // namespace

ProgramObject::ProgramObject(FILE* fp, const string& source_file_name,
                             PyObject* globals)
    : fp_(CHECK_NOTNULL(fp)),
      source_file_name_(source_file_name),
      globals_(CHECK_NOTNULL(globals)) {
}

LocalObject* ProgramObject::Clone() const {
  return new ProgramObject(fp_, source_file_name_, globals_);
}

size_t ProgramObject::Serialize(void* buffer, size_t buffer_size,
                                SerializationContext* context) const {
  ObjectProto object_proto;
  object_proto.mutable_unserializable_object();

  const size_t byte_size = static_cast<size_t>(object_proto.ByteSize());
  if (byte_size <= buffer_size) {
    object_proto.SerializeWithCachedSizesToArray(static_cast<uint8*>(buffer));
  }

  return byte_size;
}

void ProgramObject::InvokeMethod(Thread* thread,
                                 PeerObject* peer_object,
                                 const string& method_name,
                                 const vector<Value>& parameters,
                                 Value* return_value) {
  CHECK(thread != nullptr);
  CHECK_EQ(method_name, "run");
  CHECK(return_value != nullptr);

  // TODO(dss): Only read the source file once.
  rewind(fp_);

  ThreadSubstitution thread_substitution(InterpreterImpl::instance(), thread);
  {
    PythonGilLock lock;

    // TODO(dss): Add these objects to globals_.
    thread->GetOrCreateNamedObject("shared", new DictLocalObject(PyDict_New()));
    thread->GetOrCreateNamedObject("None", new NoneLocalObject());
    thread->GetOrCreateNamedObject("False", new FalseLocalObject());
    thread->GetOrCreateNamedObject("True", new TrueLocalObject());

    const object_creation_hook_func old_list_hook = Py_InstallListCreationHook(
        &WrapPythonList);

    const PythonScopedPtr return_object(
        PyRun_File(fp_, source_file_name_.c_str(), Py_file_input, globals_,
                   globals_));

    Py_InstallListCreationHook(old_list_hook);
  }

  return_value->set_empty(LOCAL_TYPE_PYOBJECT);
}

string ProgramObject::Dump() const {
  return "{ \"type\": \"ProgramObject\" }";
}

}  // namespace python
}  // namespace floating_temple
