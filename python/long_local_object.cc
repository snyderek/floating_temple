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

#include "python/long_local_object.h"

#include "third_party/Python-3.4.2/Include/Python.h"

#include <string>
#include <vector>

#include "base/integral_types.h"
#include "base/logging.h"
#include "base/string_printf.h"
#include "include/c++/value.h"
#include "python/call_method.h"
#include "python/local_object_impl.h"
#include "python/make_value.h"
#include "python/proto/serialization.pb.h"
#include "python/python_gil_lock.h"

using std::string;
using std::vector;

namespace floating_temple {

static_assert(sizeof(PY_LONG_LONG) == sizeof(int64),
              "This code assumes that the PY_LONG_LONG type is exactly 64 bits "
              "wide.");

namespace python {

LongLocalObject::LongLocalObject(PyObject* py_long_object)
    : LocalObjectImpl(CHECK_NOTNULL(py_long_object)) {
}

LocalObject* LongLocalObject::Clone() const {
  return new LongLocalObject(py_object());
}

string LongLocalObject::Dump() const {
  int overflow = 0;
  const PY_LONG_LONG value = GetLongLongValue(&overflow);

  string value_string;
  if (overflow < 0) {
    value_string = "\"(less than PY_LLONG_MIN)\"";
  } else if (overflow > 0) {
    value_string = "\"(greater than PY_LLONG_MAX)\"";
  } else {
    SStringPrintf(&value_string, "%lld", value);
  }

  return StringPrintf("{ \"type\": \"LongLocalObject\", \"value\": %s }",
                      value_string.c_str());
}

// static
LongLocalObject* LongLocalObject::ParseLongProto(const LongProto& long_proto) {
  const PY_LONG_LONG value = static_cast<PY_LONG_LONG>(long_proto.value());

  PyObject* py_long = nullptr;
  {
    PythonGilLock lock;
    py_long = PyLong_FromLongLong(value);
  }

  return new LongLocalObject(py_long);
}

void LongLocalObject::PopulateObjectProto(ObjectProto* object_proto,
                                          SerializationContext* context) const {
  CHECK(object_proto != nullptr);

  int overflow = 0;
  const PY_LONG_LONG value = GetLongLongValue(&overflow);

  // TODO(dss): Support values that are too big to fit in a 64-bit integer.
  CHECK_EQ(overflow, 0) << "Overflow";

  object_proto->mutable_long_object()->set_value(static_cast<int64>(value));
}

#define CALL_FUNCTION0(function) \
    do { \
      if (method_name == #function) { \
        CHECK_EQ(parameters.size(), 0u); \
        MakeReturnValue(function(py_object()), return_value); \
        return true; \
      } \
    } while (false)

#define CALL_FUNCTION1(function, param_type1) \
    do { \
      if (method_name == #function) { \
        CHECK_EQ(parameters.size(), 1u); \
        MakeReturnValue( \
            function( \
                py_object(), \
                ExtractValue<param_type1>(parameters[0], method_context)), \
            return_value); \
        return true; \
      } \
    } while (false)

#define CALL_FUNCTION2(function, param_type1, param_type2) \
    do { \
      if (method_name == #function) { \
        CHECK_EQ(parameters.size(), 2u); \
        MakeReturnValue( \
            function( \
                py_object(), \
                ExtractValue<param_type1>(parameters[0], method_context), \
                ExtractValue<param_type2>(parameters[1], method_context)), \
            return_value); \
        return true; \
      } \
    } while (false)

#define CALL_FUNCTION3(function, param_type1, param_type2, param_type3) \
    do { \
      if (method_name == #function) { \
        CHECK_EQ(parameters.size(), 3u); \
        MakeReturnValue( \
            function( \
                py_object(), \
                ExtractValue<param_type1>(parameters[0], method_context), \
                ExtractValue<param_type2>(parameters[1], method_context), \
                ExtractValue<param_type3>(parameters[2], method_context)), \
            return_value); \
        return true; \
      } \
    } while (false)

#define CALL_NORMAL_METHOD(method) \
    do { \
      if (method_name == #method) { \
        CallNormalMethod(py_object(), method_name, parameters, return_value); \
        return true; \
      } \
    } while (false)

#define CALL_VARARGS_METHOD(method) \
    do { \
      if (method_name == #method) { \
        CallVarargsMethod(py_object(), method_name, parameters, return_value); \
        return true; \
      } \
    } while (false)

bool LongLocalObject::InvokeTypeSpecificMethod(PeerObject* peer_object,
                                               const string& method_name,
                                               const vector<Value>& parameters,
                                               MethodContext* method_context,
                                               Value* return_value) {
  CALL_NORMAL_METHOD(conjugate);
  CALL_NORMAL_METHOD(bit_length);
  CALL_VARARGS_METHOD(to_bytes);
  CALL_NORMAL_METHOD(__trunc__);
  CALL_NORMAL_METHOD(__floor__);
  CALL_NORMAL_METHOD(__ceil__);
  CALL_VARARGS_METHOD(__round__);
  CALL_NORMAL_METHOD(__getnewargs__);
  CALL_VARARGS_METHOD(__format__);
  CALL_NORMAL_METHOD(__sizeof__);

  return false;
}

#undef CALL_FUNCTION0
#undef CALL_FUNCTION1
#undef CALL_FUNCTION2
#undef CALL_FUNCTION3
#undef CALL_NORMAL_METHOD
#undef CALL_VARARGS_METHOD

PY_LONG_LONG LongLocalObject::GetLongLongValue(int* overflow) const {
  CHECK(overflow != nullptr);

  PyObject* const py_long = py_object();

  PY_LONG_LONG value = 0;
  {
    PythonGilLock lock;

    value = PyLong_AsLongLongAndOverflow(py_long, overflow);

    if (PyErr_Occurred() != nullptr) {
      PyErr_Print();
      LOG(FATAL) << "Unexpected Python exception.";
    }
  }

  return value;
}

}  // namespace python
}  // namespace floating_temple
