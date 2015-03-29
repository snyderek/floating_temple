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
#include "python/make_value.h"
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

#define CALL_TP_METHOD0(method) \
    do { \
      if (method_name == #method) { \
        CHECK(object_type->method != nullptr); \
        CHECK_EQ(parameters.size(), 0u); \
        MakeReturnValue((*object_type->method)(py_object_), return_value); \
        return; \
      } \
    } while (false)

#define CALL_TP_METHOD1(method, param_type1) \
    do { \
      if (method_name == #method) { \
        CHECK(object_type->method != nullptr); \
        CHECK_EQ(parameters.size(), 1u); \
        MakeReturnValue( \
            (*object_type->method)( \
                py_object_, \
                ExtractValue<param_type1>(parameters[0], &method_context)), \
            return_value); \
        return; \
      } \
    } while (false)

#define CALL_TP_METHOD2(method, param_type1, param_type2) \
    do { \
      if (method_name == #method) { \
        CHECK(object_type->method != nullptr); \
        CHECK_EQ(parameters.size(), 2u); \
        MakeReturnValue( \
            (*object_type->method)( \
                py_object_, \
                ExtractValue<param_type1>(parameters[0], &method_context), \
                ExtractValue<param_type2>(parameters[1], &method_context)), \
            return_value); \
        return; \
      } \
    } while (false)

#define CALL_NB_METHOD0(method) \
    do { \
      if (method_name == #method) { \
        CHECK(object_type->tp_as_number != nullptr); \
        CHECK(object_type->tp_as_number->method != nullptr); \
        CHECK_EQ(parameters.size(), 0u); \
        MakeReturnValue((*object_type->tp_as_number->method)(py_object_), \
                        return_value); \
        return; \
      } \
    } while (false)

#define CALL_NB_METHOD1(method, param_type1) \
    do { \
      if (method_name == #method) { \
        CHECK(object_type->tp_as_number != nullptr); \
        CHECK(object_type->tp_as_number->method != nullptr); \
        CHECK_EQ(parameters.size(), 1u); \
        MakeReturnValue( \
            (*object_type->tp_as_number->method)( \
                py_object_, \
                ExtractValue<param_type1>(parameters[0], &method_context)), \
            return_value); \
        return; \
      } \
    } while (false)

#define CALL_NB_METHOD2(method, param_type1, param_type2) \
    do { \
      if (method_name == #method) { \
        CHECK(object_type->tp_as_number != nullptr); \
        CHECK(object_type->tp_as_number->method != nullptr); \
        CHECK_EQ(parameters.size(), 2u); \
        MakeReturnValue( \
            (*object_type->tp_as_number->method)( \
                py_object_, \
                ExtractValue<param_type1>(parameters[0], &method_context), \
                ExtractValue<param_type2>(parameters[1], &method_context)), \
            return_value); \
        return; \
      } \
    } while (false)

#define CALL_SQ_METHOD0(method) \
    do { \
      if (method_name == #method) { \
        CHECK(object_type->tp_as_sequence != nullptr); \
        CHECK(object_type->tp_as_sequence->method != nullptr); \
        CHECK_EQ(parameters.size(), 0u); \
        MakeReturnValue((*object_type->tp_as_sequence->method)(py_object_), \
                        return_value); \
        return; \
      } \
    } while (false)

#define CALL_SQ_METHOD1(method, param_type1) \
    do { \
      if (method_name == #method) { \
        CHECK(object_type->tp_as_sequence != nullptr); \
        CHECK(object_type->tp_as_sequence->method != nullptr); \
        CHECK_EQ(parameters.size(), 1u); \
        MakeReturnValue( \
            (*object_type->tp_as_sequence->method)( \
                py_object_, \
                ExtractValue<param_type1>(parameters[0], &method_context)), \
            return_value); \
        return; \
      } \
    } while (false)

#define CALL_SQ_METHOD2(method, param_type1, param_type2) \
    do { \
      if (method_name == #method) { \
        CHECK(object_type->tp_as_sequence != nullptr); \
        CHECK(object_type->tp_as_sequence->method != nullptr); \
        CHECK_EQ(parameters.size(), 2u); \
        MakeReturnValue( \
            (*object_type->tp_as_sequence->method)( \
                py_object_, \
                ExtractValue<param_type1>(parameters[0], &method_context), \
                ExtractValue<param_type2>(parameters[1], &method_context)), \
            return_value); \
        return; \
      } \
    } while (false)

#define CALL_MP_METHOD0(method) \
    do { \
      if (method_name == #method) { \
        CHECK(object_type->tp_as_mapping != nullptr); \
        CHECK(object_type->tp_as_mapping->method != nullptr); \
        CHECK_EQ(parameters.size(), 0u); \
        MakeReturnValue((*object_type->tp_as_mapping->method)(py_object_), \
                        return_value); \
        return; \
      } \
    } while (false)

#define CALL_MP_METHOD1(method, param_type1) \
    do { \
      if (method_name == #method) { \
        CHECK(object_type->tp_as_mapping != nullptr); \
        CHECK(object_type->tp_as_mapping->method != nullptr); \
        CHECK_EQ(parameters.size(), 1u); \
        MakeReturnValue( \
            (*object_type->tp_as_mapping->method)( \
                py_object_, \
                ExtractValue<param_type1>(parameters[0], &method_context)), \
            return_value); \
        return; \
      } \
    } while (false)

#define CALL_MP_METHOD2(method, param_type1, param_type2) \
    do { \
      if (method_name == #method) { \
        CHECK(object_type->tp_as_mapping != nullptr); \
        CHECK(object_type->tp_as_mapping->method != nullptr); \
        CHECK_EQ(parameters.size(), 2u); \
        MakeReturnValue( \
            (*object_type->tp_as_mapping->method)( \
                py_object_, \
                ExtractValue<param_type1>(parameters[0], &method_context), \
                ExtractValue<param_type2>(parameters[1], &method_context)), \
            return_value); \
        return; \
      } \
    } while (false)

void LocalObjectImpl::InvokeMethod(Thread* thread,
                                   PeerObject* peer_object,
                                   const string& method_name,
                                   const vector<Value>& parameters,
                                   Value* return_value) {
  CHECK(thread != nullptr);
  CHECK(peer_object != nullptr);

  VLOG(3) << "Invoke method on local object: " << method_name;

  MethodContext method_context;
  ThreadSubstitution thread_substitution(InterpreterImpl::instance(), thread);

  PythonGilLock lock;

  const PyTypeObject* const object_type = Py_TYPE(py_object_);
  CHECK(object_type != nullptr);

  // TODO(dss): Consider using binary search instead of linear search to find
  // the method given its name.

  // TODO(dss): Fail gracefully if the peer passes the wrong number of
  // parameters, or the wrong types of parameters.

  CALL_TP_METHOD1(tp_getattr, char*);
  CALL_TP_METHOD2(tp_setattr, char*, PyObject*);
  CALL_TP_METHOD0(tp_repr);
  CALL_TP_METHOD0(tp_hash);
  CALL_TP_METHOD2(tp_call, PyObject*, PyObject*);
  CALL_TP_METHOD0(tp_str);
  CALL_TP_METHOD1(tp_getattro, PyObject*);
  CALL_TP_METHOD2(tp_setattro, PyObject*, PyObject*);
  CALL_TP_METHOD2(tp_richcompare, PyObject*, int);
  CALL_TP_METHOD0(tp_iter);
  CALL_TP_METHOD0(tp_iternext);
  CALL_TP_METHOD2(tp_descr_get, PyObject*, PyObject*);
  CALL_TP_METHOD2(tp_descr_set, PyObject*, PyObject*);
  CALL_TP_METHOD2(tp_init, PyObject*, PyObject*);

  CALL_NB_METHOD1(nb_add, PyObject*);
  CALL_NB_METHOD1(nb_subtract, PyObject*);
  CALL_NB_METHOD1(nb_multiply, PyObject*);
  CALL_NB_METHOD1(nb_remainder, PyObject*);
  CALL_NB_METHOD1(nb_divmod, PyObject*);
  CALL_NB_METHOD2(nb_power, PyObject*, PyObject*);
  CALL_NB_METHOD0(nb_negative);
  CALL_NB_METHOD0(nb_positive);
  CALL_NB_METHOD0(nb_absolute);
  CALL_NB_METHOD0(nb_bool);
  CALL_NB_METHOD0(nb_invert);
  CALL_NB_METHOD1(nb_lshift, PyObject*);
  CALL_NB_METHOD1(nb_rshift, PyObject*);
  CALL_NB_METHOD1(nb_and, PyObject*);
  CALL_NB_METHOD1(nb_xor, PyObject*);
  CALL_NB_METHOD1(nb_or, PyObject*);
  CALL_NB_METHOD0(nb_int);
  CALL_NB_METHOD0(nb_float);
  CALL_NB_METHOD1(nb_inplace_add, PyObject*);
  CALL_NB_METHOD1(nb_inplace_subtract, PyObject*);
  CALL_NB_METHOD1(nb_inplace_multiply, PyObject*);
  CALL_NB_METHOD1(nb_inplace_remainder, PyObject*);
  CALL_NB_METHOD2(nb_inplace_power, PyObject*, PyObject*);
  CALL_NB_METHOD1(nb_inplace_lshift, PyObject*);
  CALL_NB_METHOD1(nb_inplace_rshift, PyObject*);
  CALL_NB_METHOD1(nb_inplace_and, PyObject*);
  CALL_NB_METHOD1(nb_inplace_xor, PyObject*);
  CALL_NB_METHOD1(nb_inplace_or, PyObject*);
  CALL_NB_METHOD1(nb_floor_divide, PyObject*);
  CALL_NB_METHOD1(nb_true_divide, PyObject*);
  CALL_NB_METHOD1(nb_inplace_floor_divide, PyObject*);
  CALL_NB_METHOD1(nb_inplace_true_divide, PyObject*);
  CALL_NB_METHOD0(nb_index);

  CALL_SQ_METHOD0(sq_length);
  CALL_SQ_METHOD1(sq_concat, PyObject*);
  CALL_SQ_METHOD1(sq_repeat, Py_ssize_t);
  CALL_SQ_METHOD1(sq_item, Py_ssize_t);
  CALL_SQ_METHOD2(sq_ass_item, Py_ssize_t, PyObject*);
  CALL_SQ_METHOD1(sq_contains, PyObject*);
  CALL_SQ_METHOD1(sq_inplace_concat, PyObject*);
  CALL_SQ_METHOD1(sq_inplace_repeat, Py_ssize_t);

  CALL_MP_METHOD0(mp_length);
  CALL_MP_METHOD1(mp_subscript, PyObject*);
  CALL_MP_METHOD2(mp_ass_subscript, PyObject*, PyObject*);

  // TODO(dss): Fail gracefully if a remote peer sends an invalid method name.
  LOG(FATAL) << "Unexpected method name \"" << CEscape(method_name) << "\"";
}

#undef CALL_TP_METHOD0
#undef CALL_TP_METHOD1
#undef CALL_TP_METHOD2
#undef CALL_NB_METHOD0
#undef CALL_NB_METHOD1
#undef CALL_NB_METHOD2
#undef CALL_SQ_METHOD0
#undef CALL_SQ_METHOD1
#undef CALL_SQ_METHOD2
#undef CALL_MP_METHOD0
#undef CALL_MP_METHOD1
#undef CALL_MP_METHOD2

// static
LocalObjectImpl* LocalObjectImpl::Deserialize(const void* buffer,
                                              size_t buffer_size,
                                              DeserializationContext* context) {
  CHECK(buffer != nullptr);

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
      return nullptr;

    default:
      LOG(FATAL) << "Unexpected object type: " << static_cast<int>(object_type);
  }

  return nullptr;
}

}  // namespace python
}  // namespace floating_temple
