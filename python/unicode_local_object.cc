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

#include "python/unicode_local_object.h"

#include "third_party/Python-3.4.2/Include/Python.h"

#include <string>

#include "base/escape.h"
#include "base/logging.h"
#include "base/string_printf.h"
#include "python/local_object_impl.h"
#include "python/proto/serialization.pb.h"
#include "python/python_gil_lock.h"

using std::string;

namespace floating_temple {
namespace python {
namespace {

void GetUtf8String(PyObject* py_unicode, string* utf8_string) {
  CHECK(py_unicode != nullptr);
  CHECK_NE(PyUnicode_CheckExact(py_unicode), 0);
  CHECK(utf8_string != nullptr);

  PythonGilLock lock;
  Py_ssize_t size = 0;
  const char* const data = PyUnicode_AsUTF8AndSize(py_unicode, &size);
  utf8_string->assign(data, static_cast<string::size_type>(size));
}

}  // namespace

UnicodeLocalObject::UnicodeLocalObject(PyObject* py_unicode)
    : LocalObjectImpl(CHECK_NOTNULL(py_unicode)) {
  CHECK_NE(PyUnicode_CheckExact(py_unicode), 0);
}

LocalObject* UnicodeLocalObject::Clone() const {
  return new UnicodeLocalObject(py_object());
}

string UnicodeLocalObject::Dump() const {
  string utf8_string;
  GetUtf8String(py_object(), &utf8_string);

  return StringPrintf("{ \"type\": \"UnicodeLocalObject\", \"value\": \"%s\" }",
                      CEscape(utf8_string).c_str());
}

// static
UnicodeLocalObject* UnicodeLocalObject::ParseUnicodeProto(
    const UnicodeProto& unicode_proto) {
  return new UnicodeLocalObject(
      DeserializeUnicodeObject(unicode_proto.value()));
}

void UnicodeLocalObject::PopulateObjectProto(
    ObjectProto* object_proto, SerializationContext* context) const {
  CHECK(object_proto != nullptr);
  SerializeUnicodeObject(
      py_object(), object_proto->mutable_unicode_object()->mutable_value());
}

void SerializeUnicodeObject(PyObject* in, string* out) {
  GetUtf8String(in, out);
}

PyObject* DeserializeUnicodeObject(const string& in) {
  PythonGilLock lock;

  PyObject* const py_unicode = PyUnicode_DecodeUTF8(
      in.data(), static_cast<Py_ssize_t>(in.length()), nullptr);

  if (py_unicode == nullptr) {
    CHECK(PyErr_Occurred() != nullptr);
    PyErr_Print();
    LOG(FATAL) << "Unexpected Python exception.";
  }

  return py_unicode;
}

}  // namespace python
}  // namespace floating_temple
