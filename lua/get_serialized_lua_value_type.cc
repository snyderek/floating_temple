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

#include "lua/get_serialized_lua_value_type.h"

#include "base/logging.h"
#include "lua/proto/serialization.pb.h"

namespace floating_temple {
namespace lua {

#define CHECK_FIELD(field_name, enum_const) \
  if (value_proto.has_##field_name()) { \
    CHECK_EQ(type, TValueProto::UNKNOWN); \
    type = TValueProto::enum_const; \
  }

TValueProto::Type GetSerializedLuaValueType(const TValueProto& value_proto) {
  TValueProto::Type type = TValueProto::UNKNOWN;

  CHECK_FIELD(nil, NIL);
  CHECK_FIELD(boolean, BOOLEAN);
  CHECK_FIELD(number, NUMBER);
  CHECK_FIELD(string_value, STRING);
  CHECK_FIELD(object_reference, OBJECT_REFERENCE);
  CHECK_FIELD(unserializable, UNSERIALIZABLE);

  CHECK_NE(type, TValueProto::UNKNOWN);

  return type;
}

#undef CHECK_FIELD

}  // namespace lua
}  // namespace floating_temple
