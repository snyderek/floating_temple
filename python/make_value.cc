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

#include "python/make_value.h"

#include "third_party/Python-3.4.2/Include/Python.h"

#include <cstddef>
#include <cstring>
#include <string>

#include "base/integral_types.h"
#include "base/logging.h"
#include "base/macros.h"
#include "include/c++/value.h"
#include "python/method_context.h"
#include "python/proto/local_type.pb.h"
#include "python/py_proxy_object.h"

using std::strcpy;
using std::string;

namespace floating_temple {

class PeerObject;

COMPILE_ASSERT(sizeof(int) <= sizeof(int64), int_type_is_too_big);
COMPILE_ASSERT(sizeof(long) <= sizeof(int64), long_type_is_too_big);

namespace python {

void MakeValue(int in, Value* out) {
  CHECK(out != NULL);
  out->set_int64_value(LOCAL_TYPE_INT, static_cast<int64>(in));
}

void MakeValue(long in, Value* out) {
  CHECK(out != NULL);
  out->set_int64_value(LOCAL_TYPE_LONG, static_cast<int64>(in));
}

void MakeValue(const char* in, Value* out) {
  CHECK(out != NULL);

  if (in == NULL) {
    out->set_empty(LOCAL_TYPE_CCHARP);
  } else {
    out->set_bytes_value(LOCAL_TYPE_CCHARP, in);
  }
}

void MakeValue(PyObject* in, Value* out) {
  CHECK(out != NULL);

  if (in == NULL) {
    out->set_empty(LOCAL_TYPE_PYOBJECT);
  } else {
    PeerObject* const peer_object = PyProxyObject_GetPeerObject(in);
    out->set_peer_object(LOCAL_TYPE_PYOBJECT, peer_object);
  }
}

template<>
int ExtractValue(const Value& value, MethodContext* method_context) {
  CHECK_EQ(value.local_type(), LOCAL_TYPE_INT);
  CHECK_EQ(value.type(), Value::INT64);

  // TODO(dss): Fail gracefully if the int64 value is too big to fit in an int.
  return static_cast<int>(value.int64_value());
}

template<>
long ExtractValue(const Value& value, MethodContext* method_context) {
  CHECK_EQ(value.local_type(), LOCAL_TYPE_LONG);
  CHECK_EQ(value.type(), Value::INT64);

  // TODO(dss): Fail gracefully if the int64 value is too big to fit in a long.
  return static_cast<long>(value.int64_value());
}

template<>
char* ExtractValue(const Value& value, MethodContext* method_context) {
  CHECK_EQ(value.local_type(), LOCAL_TYPE_CCHARP);
  CHECK(method_context != NULL);

  const Value::Type value_type = value.type();

  switch (value_type) {
    case Value::EMPTY:
      return NULL;

    case Value::BYTES: {
      const string& immutable_str = value.bytes_value();
      char* const str = method_context->AllocCharBuffer(
          immutable_str.length() + 1);
      strcpy(str, immutable_str.c_str());
      return str;
    }

    default:
      LOG(FATAL) << "Unexpected value type: " << static_cast<int>(value_type);
  }

  return NULL;
}

template<>
PyObject* ExtractValue(const Value& value, MethodContext* method_context) {
  CHECK_EQ(value.local_type(), LOCAL_TYPE_PYOBJECT);

  const Value::Type value_type = value.type();

  switch (value_type) {
    case Value::EMPTY:
      return NULL;

    case Value::PEER_OBJECT:
      // TODO(dss): If a proxy object already exists for the peer object, return
      // the existing proxy object instead of creating a new one. (This will
      // necessitate maintaining a map from peer objects to proxy objects.)
      return PyProxyObject_New(value.peer_object());

    default:
      LOG(FATAL) << "Unexpected value type: " << static_cast<int>(value_type);
  }

  return NULL;
}

}  // namespace python
}  // namespace floating_temple