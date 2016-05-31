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

#include "engine/convert_value.h"

#include "base/logging.h"
#include "engine/committed_value.h"
#include "engine/object_reference_impl.h"
#include "engine/proto/uuid.pb.h"
#include "engine/proto/value_proto.pb.h"
#include "engine/shared_object.h"
#include "include/c++/value.h"

namespace floating_temple {
namespace engine {

#define CONVERT_FIELD(enum_const, setter_method, getter_method) \
  case CommittedValue::enum_const: \
    out->setter_method(in.getter_method()); \
    break;

void ConvertCommittedValueToValueProto(const CommittedValue& in,
                                       ValueProto* out) {
  CHECK(out != nullptr);

  out->Clear();
  out->set_local_type(in.local_type());

  const CommittedValue::Type type = in.type();

  switch (type) {
    case CommittedValue::EMPTY:
      out->mutable_empty_value();
      break;

    CONVERT_FIELD(DOUBLE, set_double_value, double_value);
    CONVERT_FIELD(FLOAT, set_float_value, float_value);
    CONVERT_FIELD(INT64, set_int64_value, int64_value);
    CONVERT_FIELD(UINT64, set_uint64_value, uint64_value);
    CONVERT_FIELD(BOOL, set_bool_value, bool_value);
    CONVERT_FIELD(STRING, set_string_value, string_value);
    CONVERT_FIELD(BYTES, set_bytes_value, bytes_value);

    case CommittedValue::SHARED_OBJECT: {
      out->mutable_object_id()->CopyFrom(in.shared_object()->object_id());
      break;
    }

    default:
      LOG(FATAL) << "Unexpected committed value type: "
                 << static_cast<int>(type);
  }
}

#undef CONVERT_FIELD

#define CONVERT_FIELD(enum_const, setter_method, getter_method) \
  case CommittedValue::enum_const: \
    out->setter_method(local_type, in.getter_method()); \
    break;

void ConvertCommittedValueToValue(const CommittedValue& in, Value* out) {
  CHECK(out != nullptr);

  const int local_type = in.local_type();
  const CommittedValue::Type type = in.type();

  switch (type) {
    case CommittedValue::EMPTY:
      out->set_empty(local_type);
      break;

    CONVERT_FIELD(DOUBLE, set_double_value, double_value);
    CONVERT_FIELD(FLOAT, set_float_value, float_value);
    CONVERT_FIELD(INT64, set_int64_value, int64_value);
    CONVERT_FIELD(UINT64, set_uint64_value, uint64_value);
    CONVERT_FIELD(BOOL, set_bool_value, bool_value);
    CONVERT_FIELD(STRING, set_string_value, string_value);
    CONVERT_FIELD(BYTES, set_bytes_value, bytes_value);

    case CommittedValue::SHARED_OBJECT:
      out->set_object_reference(
          local_type, in.shared_object()->GetOrCreateObjectReference());
      break;

    default:
      LOG(FATAL) << "Unexpected committed value type: "
                 << static_cast<int>(type);
  }
}

#undef CONVERT_FIELD

}  // namespace engine
}  // namespace floating_temple
