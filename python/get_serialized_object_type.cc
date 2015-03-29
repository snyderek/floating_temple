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

#include "python/get_serialized_object_type.h"

#include "base/logging.h"
#include "python/proto/serialization.pb.h"

namespace floating_temple {
namespace python {

#define CHECK_FIELD(field_name, enum_const) \
  if (object_proto.has_##field_name()) { \
    CHECK_EQ(type, ObjectProto::UNKNOWN); \
    type = ObjectProto::enum_const; \
  }

ObjectProto::Type GetSerializedObjectType(const ObjectProto& object_proto) {
  ObjectProto::Type type = ObjectProto::UNKNOWN;

  CHECK_FIELD(py_none_object, PY_NONE);
  CHECK_FIELD(long_object, LONG);
  CHECK_FIELD(false_object, FALSE);
  CHECK_FIELD(true_object, TRUE);
  CHECK_FIELD(float_object, FLOAT);
  CHECK_FIELD(complex_object, COMPLEX);
  CHECK_FIELD(bytes_object, BYTES);
  CHECK_FIELD(byte_array_object, BYTE_ARRAY);
  CHECK_FIELD(unicode_object, UNICODE);
  CHECK_FIELD(tuple_object, TUPLE);
  CHECK_FIELD(list_object, LIST);
  CHECK_FIELD(dict_object, DICT);
  CHECK_FIELD(set_object, SET);
  CHECK_FIELD(frozen_set_object, FROZEN_SET);
  CHECK_FIELD(unserializable_object, UNSERIALIZABLE);

  CHECK_NE(type, ObjectProto::UNKNOWN);

  return type;
}

#undef CHECK_FIELD

}  // namespace python
}  // namespace floating_temple
