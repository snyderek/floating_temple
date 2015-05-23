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

#include <cstddef>
#include <memory>
#include <string>

#include "base/logging.h"
#include "base/string_printf.h"
#include "python/local_object_impl.h"
#include "python/proto/serialization.pb.h"
#include "python/python_gil_lock.h"

using std::size_t;
using std::string;
using std::unique_ptr;

namespace floating_temple {
namespace python {
namespace {

const bool kSerializedFormIsLittleEndian = false;
const bool kSerializedFormIsSigned = true;

}  // namespace

LongLocalObject::LongLocalObject(PyObject* py_long_object)
    : LocalObjectImpl(CHECK_NOTNULL(py_long_object)) {
}

VersionedLocalObject* LongLocalObject::Clone() const {
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
  return new LongLocalObject(DeserializeLongObject(long_proto.value_bytes()));
}

void LongLocalObject::PopulateObjectProto(ObjectProto* object_proto,
                                          SerializationContext* context) const {
  CHECK(object_proto != nullptr);
  SerializeLongObject(
      py_object(), object_proto->mutable_long_object()->mutable_value_bytes());
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

void SerializeLongObject(PyObject* in, string* out) {
  CHECK(in != nullptr);
  CHECK_NE(PyLong_CheckExact(in), 0);
  CHECK(out != nullptr);

  PythonGilLock lock;

  const size_t num_bits = _PyLong_NumBits(in);
  if (num_bits == 0) {
    out->clear();
    return;
  }

  // size == ceil((num_bits + 1) / 8) == floor(num_bits / 8) + 1
  const size_t size = num_bits / 8u + 1u;

  const unique_ptr<unsigned char[]> buffer(new unsigned char[size]);
  if (_PyLong_AsByteArray(reinterpret_cast<PyLongObject*>(in), buffer.get(),
                          size, kSerializedFormIsLittleEndian ? 1 : 0,
                          kSerializedFormIsSigned ? 1 : 0) != 0) {
    CHECK(PyErr_Occurred() != nullptr);
    PyErr_Print();
    LOG(FATAL) << "Unexpected Python exception.";
  }

  out->assign(reinterpret_cast<const char*>(buffer.get()), size);
}

PyObject* DeserializeLongObject(const string& in) {
  PythonGilLock lock;
  return _PyLong_FromByteArray(
      reinterpret_cast<const unsigned char*>(in.data()), in.length(),
      kSerializedFormIsLittleEndian ? 1 : 0, kSerializedFormIsSigned ? 1 : 0);
}

}  // namespace python
}  // namespace floating_temple
