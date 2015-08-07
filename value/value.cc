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

#include "include/c++/value.h"

#include <cinttypes>
#include <string>

#include "base/escape.h"
#include "base/integral_types.h"
#include "base/logging.h"
#include "include/c++/object_reference.h"
#include "util/dump_context.h"

using std::string;

namespace floating_temple {

#define ASSIGN_VALUE(enum_const, field_name) \
  case enum_const: \
    field_name = other.field_name; \
    break;

Value::Value(const Value& other)
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

    case OBJECT_REFERENCE:
      object_reference_ = other.object_reference_;
      break;

    default:
      LOG(FATAL) << "Unexpected value type: " << static_cast<int>(other.type_);
  }
}

Value& Value::operator=(const Value& other) {
  ChangeType(other.local_type_, other.type_);

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

    case OBJECT_REFERENCE:
      object_reference_ = other.object_reference_;
      break;

    default:
      LOG(FATAL) << "Unexpected value type: " << static_cast<int>(other.type_);
  }

  return *this;
}

#undef ASSIGN_VALUE

void Value::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  // TODO(dss): Don't use strings to represent non-string types, because it's
  // ambiguous. For example, '"EMPTY"' could be either an EMPTY value or a
  // STRING value.
  switch (type_) {
    case UNINITIALIZED:
      dc->AddString("UNINITIALIZED");
      break;

    case EMPTY:
      dc->AddString("EMPTY");
      break;

    case DOUBLE:
      dc->AddDouble(double_value_);
      break;

    case FLOAT:
      dc->AddFloat(float_value_);
      break;

    case INT64:
      dc->AddInt64(int64_value_);
      break;

    case UINT64:
      dc->AddUint64(uint64_value_);
      break;

    case BOOL:
      dc->AddBool(bool_value_);
      break;

    case STRING:
    case BYTES:
      CHECK(string_or_bytes_value_ != nullptr);
      dc->AddString(*string_or_bytes_value_);
      break;

    case OBJECT_REFERENCE:
      CHECK(object_reference_ != nullptr);
      object_reference_->Dump(dc);
      break;

    default:
      LOG(FATAL) << "Unexpected value type: " << static_cast<int>(type_);
  }
}

void Value::ChangeType(int local_type, Type new_type) {
  local_type_ = local_type;

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

}  // namespace floating_temple
