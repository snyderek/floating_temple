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

#include "toy_lang/get_serialized_expression_type.h"

#include "base/logging.h"
#include "toy_lang/proto/serialization.pb.h"

namespace floating_temple {
namespace toy_lang {

#define CHECK_FIELD(has_method, enum_const) \
  if (expression_proto.has_method()) { \
    CHECK_EQ(type, ExpressionProto::UNKNOWN); \
    type = ExpressionProto::enum_const; \
  }

ExpressionProto::Type GetSerializedExpressionType(
    const ExpressionProto& expression_proto) {
  ExpressionProto::Type type = ExpressionProto::UNKNOWN;

  CHECK_FIELD(has_int_expression, INT);
  CHECK_FIELD(has_string_expression, STRING);
  CHECK_FIELD(has_symbol_expression, SYMBOL);
  CHECK_FIELD(has_expression_expression, EXPRESSION);
  CHECK_FIELD(has_function_expression, FUNCTION);
  CHECK_FIELD(has_list_expression, LIST);

  CHECK_NE(type, ExpressionProto::UNKNOWN);

  return type;
}

#undef CHECK_FIELD

}  // namespace toy_lang
}  // namespace floating_temple
