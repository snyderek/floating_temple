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

#include "engine/value_proto_util.h"

#include <string>

#include "base/logging.h"
#include "engine/proto/value_proto.pb.h"
#include "engine/uuid_util.h"

namespace floating_temple {
namespace engine {

#define CHECK_FIELD(has_method, enum_const) \
  if (value.has_method()) { \
    CHECK_EQ(type, ValueProto::UNKNOWN); \
    type = ValueProto::enum_const; \
  }

ValueProto::Type GetValueProtoType(const ValueProto& value) {
  ValueProto::Type type = ValueProto::UNKNOWN;

  CHECK_FIELD(has_empty_value, EMPTY);
  CHECK_FIELD(has_double_value, DOUBLE);
  CHECK_FIELD(has_float_value, FLOAT);
  CHECK_FIELD(has_int64_value, INT64);
  CHECK_FIELD(has_uint64_value, UINT64);
  CHECK_FIELD(has_bool_value, BOOL);
  CHECK_FIELD(has_string_value, STRING);
  CHECK_FIELD(has_bytes_value, BYTES);
  CHECK_FIELD(has_object_id, OBJECT_ID);

  CHECK_NE(type, ValueProto::UNKNOWN);

  return type;
}

#undef CHECK_FIELD

#define COMPARE_FIELDS(enum_const, field_name) \
  case ValueProto::enum_const: \
    return a.field_name() == b.field_name();

bool ValueProtosEqual(const ValueProto& a, const ValueProto& b) {
  if (a.local_type() != b.local_type()) {
    return false;
  }

  const ValueProto::Type a_type = GetValueProtoType(a);
  const ValueProto::Type b_type = GetValueProtoType(b);

  if (a_type != b_type) {
    return false;
  }

  switch (a_type) {
    case ValueProto::EMPTY:
      return true;

    COMPARE_FIELDS(DOUBLE, double_value);
    COMPARE_FIELDS(FLOAT, float_value);
    COMPARE_FIELDS(INT64, int64_value);
    COMPARE_FIELDS(UINT64, uint64_value);
    COMPARE_FIELDS(BOOL, bool_value);
    COMPARE_FIELDS(STRING, string_value);
    COMPARE_FIELDS(BYTES, bytes_value);

    case ValueProto::OBJECT_ID:
      return CompareUuids(a.object_id(), b.object_id()) == 0;

    default:
      LOG(FATAL) << "Unexpected value type: " << static_cast<int>(a_type);
  }
}

#undef COMPARE_FIELDS

}  // namespace engine
}  // namespace floating_temple
