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

#include "peer/committed_value.h"

#include <cinttypes>
#include <string>

#include "base/escape.h"
#include "base/integral_types.h"
#include "base/logging.h"
#include "base/string_printf.h"
#include "peer/shared_object.h"
#include "peer/uuid_util.h"

using std::string;

namespace floating_temple {
namespace peer {

#define ASSIGN_VALUE(enum_const, field_name) \
  case enum_const: \
    field_name = other.field_name; \
    break;

CommittedValue::CommittedValue(const CommittedValue& other)
    : local_type_(other.local_type_),
      type_(other.type_) {
  switch (other.type_) {
    case UNINITIALIZED:
    case EMPTY:
      break;

    ASSIGN_VALUE(DOUBLE, double_value_);
    ASSIGN_VALUE(FLOAT, float_value_);
    ASSIGN_VALUE(INT64, int64_value_);
    ASSIGN_VALUE(UINT64, uint64_value_);
    ASSIGN_VALUE(BOOL, bool_value_);

    case STRING:
    case BYTES:
      string_or_bytes_value_ = new string(*other.string_or_bytes_value_);
      break;

    case SHARED_OBJECT:
      shared_object_ = other.shared_object_;
      break;

    default:
      LOG(FATAL) << "Unexpected committed value type: "
                 << static_cast<int>(other.type_);
  }
}

CommittedValue& CommittedValue::operator=(const CommittedValue& other) {
  local_type_ = other.local_type_;
  ChangeType(other.type_);

  switch (other.type_) {
    case UNINITIALIZED:
    case EMPTY:
      break;

    ASSIGN_VALUE(DOUBLE, double_value_);
    ASSIGN_VALUE(FLOAT, float_value_);
    ASSIGN_VALUE(INT64, int64_value_);
    ASSIGN_VALUE(UINT64, uint64_value_);
    ASSIGN_VALUE(BOOL, bool_value_);

    case STRING:
    case BYTES:
      *string_or_bytes_value_ = *other.string_or_bytes_value_;
      break;

    case SHARED_OBJECT:
      shared_object_ = other.shared_object_;
      break;

    default:
      LOG(FATAL) << "Unexpected committed value type: "
                 << static_cast<int>(other.type_);
  }

  return *this;
}

#undef ASSIGN_VALUE

string CommittedValue::Dump() const {
  switch (type_) {
    case UNINITIALIZED:
      return "\"UNINITIALIZED\"";

    case EMPTY:
      return "\"EMPTY\"";

    case DOUBLE:
      return StringPrintf("%f", double_value_);

    case FLOAT:
      return StringPrintf("%f", static_cast<double>(float_value_));

    case INT64:
      return StringPrintf("%" PRId64, int64_value_);

    case UINT64:
      return StringPrintf("%" PRIu64, uint64_value_);

    case BOOL:
      if (bool_value_) {
        return "true";
      } else {
        return "false";
      }

    case STRING:
    case BYTES:
      CHECK(string_or_bytes_value_ != nullptr);
      return StringPrintf("\"%s\"", CEscape(*string_or_bytes_value_).c_str());

    case SHARED_OBJECT:
      CHECK(shared_object_ != nullptr);
      return StringPrintf("{ \"object_id\": \"%s\" }",
                          UuidToString(shared_object_->object_id()).c_str());

    default:
      LOG(FATAL) << "Unexpected value type: " << static_cast<int>(type_);
  }

  return "";
}

void CommittedValue::ChangeType(Type new_type) {
  if (type_ == STRING || type_ == BYTES) {
    if (new_type != STRING && new_type != BYTES) {
      delete string_or_bytes_value_;
    }
  } else {
    if (new_type == STRING || new_type == BYTES) {
      string_or_bytes_value_ = new string();
    }
  }

  type_ = new_type;
}

}  // namespace peer
}  // namespace floating_temple
