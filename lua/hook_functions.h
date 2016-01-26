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

#ifndef LUA_HOOK_FUNCTIONS_H_
#define LUA_HOOK_FUNCTIONS_H_

struct lua_State;
struct lua_TValue;

namespace floating_temple {
namespace lua {

// Returns non-zero if the objects are equal.
int AreObjectsEqual(const void* ft_obj1, const void* ft_obj2);

// Each of these functions returns non-zero if it performed the requested
// operation. If a function returns zero, the caller should fall back to the
// default behavior of the stock Lua interpreter.
int CreateTable(lua_State* lua_state, lua_TValue* obj, int b, int c);
int CallMethod_GetTable(lua_State* lua_state, const lua_TValue* table,
                        const lua_TValue* key, lua_TValue* val);
int CallMethod_SetTable(lua_State* lua_state, const lua_TValue* table,
                        const lua_TValue* key, const lua_TValue* val);
int CallMethod_ObjLen(lua_State* lua_state, lua_TValue* ra,
                      const lua_TValue* rb);
int CallMethod_SetList(lua_State* lua_state, const lua_TValue* ra, int n,
                       int c);
int CallMethod_TableInsert(lua_State* lua_state, const lua_TValue* table,
                           int pos, const lua_TValue* value);

}  // namespace lua
}  // namespace floating_temple

#endif  // LUA_HOOK_FUNCTIONS_H_
