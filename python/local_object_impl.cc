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

#include "python/local_object_impl.h"

#include "third_party/Python-3.4.2/Include/Python.h"

#include <cstddef>
#include <string>
#include <vector>

#include "base/escape.h"
#include "base/integral_types.h"
#include "base/logging.h"
#include "include/c++/value.h"
#include "python/dict_local_object.h"
#include "python/false_local_object.h"
#include "python/get_serialized_object_type.h"
#include "python/interpreter_impl.h"
#include "python/list_local_object.h"
#include "python/long_local_object.h"
#include "python/method_context.h"
#include "python/none_local_object.h"
#include "python/proto/serialization.pb.h"
#include "python/python_gil_lock.h"
#include "python/thread_substitution.h"
#include "python/true_local_object.h"

using std::size_t;
using std::string;
using std::vector;

namespace floating_temple {
namespace python {

LocalObjectImpl::LocalObjectImpl(PyObject* py_object)
    : py_object_(CHECK_NOTNULL(py_object)) {
}

LocalObjectImpl::~LocalObjectImpl() {
  PythonGilLock lock;
  Py_DECREF(py_object_);
}

size_t LocalObjectImpl::Serialize(void* buffer, size_t buffer_size,
                                  SerializationContext* context) const {
  ObjectProto object_proto;
  PopulateObjectProto(&object_proto, context);

  const size_t byte_size = static_cast<size_t>(object_proto.ByteSize());
  if (byte_size <= buffer_size) {
    object_proto.SerializeWithCachedSizesToArray(static_cast<uint8*>(buffer));
  }

  return byte_size;
}

void LocalObjectImpl::InvokeMethod(Thread* thread,
                                   PeerObject* peer_object,
                                   const string& method_name,
                                   const vector<Value>& parameters,
                                   Value* return_value) {
  CHECK(thread != NULL);
  CHECK(peer_object != NULL);

  VLOG(3) << "Invoke method on local object: " << method_name;

  MethodContext method_context;
  ThreadSubstitution thread_substitution(InterpreterImpl::instance(), thread);

  PythonGilLock lock;

  // TODO(dss): Fail gracefully if the peer passes the wrong number of
  // parameters, or the wrong types of parameters.

  // TODO(dss): Fail gracefully if a remote peer sends an invalid method name.
  CHECK(this->InvokeTypeSpecificMethod(peer_object, method_name, parameters,
                                       &method_context, return_value))
      << "Unexpected method name \"" << CEscape(method_name) << "\"";
}

// static
LocalObjectImpl* LocalObjectImpl::Deserialize(const void* buffer,
                                              size_t buffer_size,
                                              DeserializationContext* context) {
  CHECK(buffer != NULL);

  ObjectProto object_proto;
  CHECK(object_proto.ParseFromArray(buffer, buffer_size));

  const ObjectProto::Type object_type = GetSerializedObjectType(object_proto);

  switch (object_type) {
    case ObjectProto::PY_NONE:
      return new NoneLocalObject();

    case ObjectProto::LONG:
      return LongLocalObject::ParseLongProto(object_proto.long_object());

    case ObjectProto::FALSE:
      return new FalseLocalObject();

    case ObjectProto::TRUE:
      return new TrueLocalObject();

    case ObjectProto::LIST:
      return ListLocalObject::ParseListProto(object_proto.list_object(),
                                             context);

    case ObjectProto::DICT:
      return DictLocalObject::ParseDictProto(object_proto.dict_object(),
                                             context);

    case ObjectProto::FLOAT:
    case ObjectProto::COMPLEX:
    case ObjectProto::BYTES:
    case ObjectProto::BYTE_ARRAY:
    case ObjectProto::UNICODE:
    case ObjectProto::TUPLE:
    case ObjectProto::SET:
    case ObjectProto::FROZEN_SET:
      LOG(FATAL) << "Not yet implemented";
      return NULL;

    default:
      LOG(FATAL) << "Unexpected object type: " << static_cast<int>(object_type);
  }

  return NULL;
}

}  // namespace python
}  // namespace floating_temple
