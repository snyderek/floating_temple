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

#include "toy_lang/get_serialized_object_type.h"

#include "base/logging.h"
#include "toy_lang/proto/serialization.pb.h"

namespace floating_temple {
namespace toy_lang {

#define CHECK_FIELD(has_method, enum_const) \
  if (object_proto.has_method()) { \
    CHECK_EQ(type, ObjectProto::UNKNOWN); \
    type = ObjectProto::enum_const; \
  }

ObjectProto::Type GetSerializedObjectType(const ObjectProto& object_proto) {
  ObjectProto::Type type = ObjectProto::UNKNOWN;

  CHECK_FIELD(has_none_object, NONE);
  CHECK_FIELD(has_bool_object, BOOL);
  CHECK_FIELD(has_int_object, INT);
  CHECK_FIELD(has_string_object, STRING);
  CHECK_FIELD(has_symbol_object, SYMBOL);
  CHECK_FIELD(has_variable_object, VARIABLE);
  CHECK_FIELD(has_expression_object, EXPRESSION);
  CHECK_FIELD(has_list_object, LIST);
  CHECK_FIELD(has_map_object, MAP);
  CHECK_FIELD(has_range_iterator_object, RANGE_ITERATOR);
  CHECK_FIELD(has_list_function, LIST_FUNCTION);
  CHECK_FIELD(has_set_variable_function, SET_VARIABLE_FUNCTION);
  CHECK_FIELD(has_for_function, FOR_FUNCTION);
  CHECK_FIELD(has_range_function, RANGE_FUNCTION);
  CHECK_FIELD(has_print_function, PRINT_FUNCTION);
  CHECK_FIELD(has_add_function, ADD_FUNCTION);
  CHECK_FIELD(has_begin_tran_function, BEGIN_TRAN_FUNCTION);
  CHECK_FIELD(has_end_tran_function, END_TRAN_FUNCTION);
  CHECK_FIELD(has_if_function, IF_FUNCTION);
  CHECK_FIELD(has_not_function, NOT_FUNCTION);
  CHECK_FIELD(has_while_function, WHILE_FUNCTION);
  CHECK_FIELD(has_less_than_function, LESS_THAN_FUNCTION);
  CHECK_FIELD(has_len_function, LEN_FUNCTION);
  CHECK_FIELD(has_list_append_function, LIST_APPEND_FUNCTION);
  CHECK_FIELD(has_list_get_function, LIST_GET_FUNCTION);
  CHECK_FIELD(has_map_is_set_function, MAP_IS_SET_FUNCTION);
  CHECK_FIELD(has_map_get_function, MAP_GET_FUNCTION);
  CHECK_FIELD(has_map_set_function, MAP_SET_FUNCTION);

  CHECK_NE(type, ObjectProto::UNKNOWN);

  return type;
}

#undef CHECK_FIELD

}  // namespace toy_lang
}  // namespace floating_temple
