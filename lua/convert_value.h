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

#ifndef LUA_CONVERT_VALUE_H_
#define LUA_CONVERT_VALUE_H_

struct lua_State;
struct lua_TValue;

namespace floating_temple {

class DeserializationContext;
class SerializationContext;
class Value;

namespace lua {

class TValueProto;

void LuaValueToValue(const lua_TValue* lua_value, Value* value);
void ValueToLuaValue(lua_State* lua_state, const Value& value,
                     lua_TValue* lua_value);

void LuaValueToValueProto(const lua_TValue* lua_value, TValueProto* value_proto,
                          SerializationContext* context);
void ValueProtoToLuaValue(lua_State* lua_state,
                          const TValueProto& value_proto,
                          lua_TValue* lua_value,
                          DeserializationContext* context);

}  // namespace lua
}  // namespace floating_temple

#endif  // LUA_CONVERT_VALUE_H_
