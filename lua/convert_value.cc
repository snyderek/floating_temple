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

#include "lua/convert_value.h"

#include <string>

#include "base/integral_types.h"
#include "base/logging.h"
#include "include/c++/deserialization_context.h"
#include "include/c++/serialization_context.h"
#include "include/c++/value.h"
#include "lua/get_serialized_lua_value_type.h"
#include "lua/proto/serialization.pb.h"
#include "third_party/lua-5.2.3/src/lobject.h"
#include "third_party/lua-5.2.3/src/lstring.h"
#include "third_party/lua-5.2.3/src/lua.h"

using std::string;

namespace floating_temple {

class ObjectReference;

namespace lua {

void LuaValueToValue(const TValue* lua_value, Value* value) {
  CHECK(lua_value != nullptr);
  CHECK(value != nullptr);

  const int lua_type = ttypenv(lua_value);

  switch (lua_type) {
    case LUA_TNIL:
      value->set_empty(lua_type);
      break;

    case LUA_TBOOLEAN:
      value->set_bool_value(lua_type, (bvalue(lua_value) != 0));
      break;

    case LUA_TNUMBER:
      value->set_double_value(lua_type, nvalue(lua_value));
      break;

    case LUA_TSTRING: {
      const TString* const lua_string = rawtsvalue(lua_value);
      value->set_string_value(lua_type,
                              string(getstr(lua_string), lua_string->tsv.len));
      break;
    }

    case LUA_TOBJECTREFERENCE:
      value->set_object_reference(
          lua_type,
          *reinterpret_cast<ObjectReference* const*>(&val_(lua_value).obj_ref));
      break;

    default:
      LOG(FATAL) << "Unexpected lua value type: " << lua_type;
  }
}

void ValueToLuaValue(lua_State* lua_state, const Value& value,
                     TValue* lua_value) {
  CHECK(lua_state != nullptr);
  CHECK(lua_value != nullptr);

  const int lua_type = value.local_type();

  switch (lua_type) {
    case LUA_TNIL:
      setnilvalue(lua_value);
      break;

    case LUA_TBOOLEAN:
      setbvalue(lua_value, value.bool_value() ? 1 : 0);
      break;

    case LUA_TNUMBER:
      setnvalue(lua_value, value.double_value());
      break;

    case LUA_TSTRING: {
      const string& s = value.string_value();
      TString* const lua_string = luaS_newlstr(lua_state, s.data(), s.length());
      setsvalue(lua_state, lua_value, lua_string);
      break;
    }

    case LUA_TOBJECTREFERENCE:
      *reinterpret_cast<ObjectReference**>(&val_(lua_value).obj_ref) =
          value.object_reference();
      settt_(lua_value, lua_type);
      break;

    default:
      LOG(FATAL) << "Unexpected lua value type: " << lua_type;
  }
}

void LuaValueToValueProto(const TValue* lua_value, TValueProto* value_proto,
                          SerializationContext* context) {
  CHECK(lua_value != nullptr);
  CHECK(value_proto != nullptr);
  CHECK(context != nullptr);

  const int lua_type = ttypenv(lua_value);

  switch (lua_type) {
    case LUA_TNIL:
      value_proto->mutable_nil();
      break;

    case LUA_TBOOLEAN:
      value_proto->mutable_boolean()->set_value(bvalue(lua_value) != 0);
      break;

    case LUA_TNUMBER:
      value_proto->mutable_number()->set_value(nvalue(lua_value));
      break;

    case LUA_TSTRING: {
      const TString* const lua_string = rawtsvalue(lua_value);
      value_proto->mutable_string_value()->set_value(
          getstr(lua_string), static_cast<int>(lua_string->tsv.len));
      break;
    }

    case LUA_TOBJECTREFERENCE: {
      ObjectReference* const object_reference =
          *reinterpret_cast<ObjectReference* const*>(&val_(lua_value).obj_ref);
      const int object_index = context->GetIndexForObjectReference(
          object_reference);
      value_proto->mutable_object_reference()->set_object_index(
          static_cast<int64>(object_index));
      break;
    }

    default:
      LOG(FATAL) << "Unexpected lua value type: " << lua_type;
  }
}

void ValueProtoToLuaValue(lua_State* lua_state,
                          const TValueProto& value_proto,
                          TValue* lua_value,
                          DeserializationContext* context) {
  CHECK(lua_state != nullptr);
  CHECK(lua_value != nullptr);
  CHECK(context != nullptr);

  const TValueProto::Type lua_type = GetSerializedLuaValueType(value_proto);

  switch (lua_type) {
    case TValueProto::NIL:
      setnilvalue(lua_value);
      break;

    case TValueProto::BOOLEAN:
      setbvalue(lua_value, value_proto.boolean().value() ? 1 : 0);
      break;

    case TValueProto::NUMBER:
      setnvalue(lua_value, value_proto.number().value());
      break;

    case TValueProto::STRING: {
      const string& s = value_proto.string_value().value();
      TString* const lua_string = luaS_newlstr(lua_state, s.data(), s.length());
      setsvalue(lua_state, lua_value, lua_string);
      break;
    }

    case TValueProto::OBJECT_REFERENCE:
      *reinterpret_cast<ObjectReference**>(&val_(lua_value).obj_ref) =
          context->GetObjectReferenceByIndex(
              value_proto.object_reference().object_index());
      settt_(lua_value, lua_type);
      break;

    default:
      LOG(FATAL) << "Unexpected lua value type: " << lua_type;
  }
}

}  // namespace lua
}  // namespace floating_temple
