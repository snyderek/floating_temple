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

#include "toy_lang/zoo/local_object_impl.h"

#include <cstddef>

#include "base/logging.h"
#include "toy_lang/get_serialized_object_type.h"
#include "toy_lang/proto/serialization.pb.h"
#include "toy_lang/zoo/add_function.h"
#include "toy_lang/zoo/begin_tran_function.h"
#include "toy_lang/zoo/bool_object.h"
#include "toy_lang/zoo/end_tran_function.h"
#include "toy_lang/zoo/expression_object.h"
#include "toy_lang/zoo/for_function.h"
#include "toy_lang/zoo/function.h"
#include "toy_lang/zoo/if_function.h"
#include "toy_lang/zoo/int_object.h"
#include "toy_lang/zoo/len_function.h"
#include "toy_lang/zoo/less_than_function.h"
#include "toy_lang/zoo/list_append_function.h"
#include "toy_lang/zoo/list_function.h"
#include "toy_lang/zoo/list_get_function.h"
#include "toy_lang/zoo/list_object.h"
#include "toy_lang/zoo/map_get_function.h"
#include "toy_lang/zoo/map_is_set_function.h"
#include "toy_lang/zoo/map_object.h"
#include "toy_lang/zoo/map_set_function.h"
#include "toy_lang/zoo/none_object.h"
#include "toy_lang/zoo/not_function.h"
#include "toy_lang/zoo/print_function.h"
#include "toy_lang/zoo/range_function.h"
#include "toy_lang/zoo/range_iterator_object.h"
#include "toy_lang/zoo/set_variable_function.h"
#include "toy_lang/zoo/string_object.h"
#include "toy_lang/zoo/while_function.h"

using std::size_t;

namespace floating_temple {
namespace toy_lang {

size_t LocalObjectImpl::Serialize(void* buffer, size_t buffer_size,
                                  SerializationContext* context) const {
  ObjectProto object_proto;
  // TODO(dss): Use the 'this' keyword when calling a pure virtual method on
  // this object.
  PopulateObjectProto(&object_proto, context);

  const size_t byte_size = static_cast<size_t>(object_proto.ByteSize());
  if (byte_size <= buffer_size) {
    object_proto.SerializeWithCachedSizesToArray(static_cast<uint8*>(buffer));
  }

  return byte_size;
}

// static
LocalObjectImpl* LocalObjectImpl::Deserialize(const void* buffer,
                                              size_t buffer_size,
                                              DeserializationContext* context) {
  CHECK(buffer != nullptr);

  ObjectProto object_proto;
  CHECK(object_proto.ParseFromArray(buffer, buffer_size));

  const ObjectProto::Type object_type = GetSerializedObjectType(object_proto);

  switch (object_type) {
    case ObjectProto::NONE:
      return new NoneObject();

    case ObjectProto::BOOL:
      return BoolObject::ParseBoolProto(object_proto.bool_object());

    case ObjectProto::INT:
      return IntObject::ParseIntProto(object_proto.int_object());

    case ObjectProto::STRING:
      return StringObject::ParseStringProto(object_proto.string_object());

    case ObjectProto::EXPRESSION:
      return ExpressionObject::ParseExpressionObjectProto(
          object_proto.expression_object(), context);

    case ObjectProto::LIST:
      return ListObject::ParseListProto(object_proto.list_object(), context);

    case ObjectProto::MAP:
      return MapObject::ParseMapProto(object_proto.map_object(), context);

    case ObjectProto::RANGE_ITERATOR:
      return RangeIteratorObject::ParseRangeIteratorProto(
          object_proto.range_iterator_object());

    case ObjectProto::LIST_FUNCTION:
      return new ListFunction();

    case ObjectProto::SET_VARIABLE_FUNCTION:
      return new SetVariableFunction();

    case ObjectProto::FOR_FUNCTION:
      return new ForFunction();

    case ObjectProto::RANGE_FUNCTION:
      return new RangeFunction();

    case ObjectProto::PRINT_FUNCTION:
      return new PrintFunction();

    case ObjectProto::ADD_FUNCTION:
      return new AddFunction();

    case ObjectProto::BEGIN_TRAN_FUNCTION:
      return new BeginTranFunction();

    case ObjectProto::END_TRAN_FUNCTION:
      return new EndTranFunction();

    case ObjectProto::IF_FUNCTION:
      return new IfFunction();

    case ObjectProto::NOT_FUNCTION:
      return new NotFunction();

    case ObjectProto::WHILE_FUNCTION:
      return new WhileFunction();

    case ObjectProto::LESS_THAN_FUNCTION:
      return new LessThanFunction();

    case ObjectProto::LEN_FUNCTION:
      return new LenFunction();

    case ObjectProto::LIST_APPEND_FUNCTION:
      return new ListAppendFunction();

    case ObjectProto::LIST_GET_FUNCTION:
      return new ListGetFunction();

    case ObjectProto::MAP_IS_SET_FUNCTION:
      return new MapIsSetFunction();

    case ObjectProto::MAP_GET_FUNCTION:
      return new MapGetFunction();

    case ObjectProto::MAP_SET_FUNCTION:
      return new MapSetFunction();

    default:
      LOG(FATAL) << "Unexpected object type: " << static_cast<int>(object_type);
  }

  return nullptr;
}

}  // namespace toy_lang
}  // namespace floating_temple
