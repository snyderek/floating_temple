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

#include "base/integral_types.h"
#include "base/logging.h"
#include "base/string_printf.h"
#include "python/local_object_impl.h"
#include "python/proto/serialization.pb.h"
#include "python/python_gil_lock.h"

using std::string;

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
