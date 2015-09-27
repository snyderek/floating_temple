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

#include "lua/hook_functions.h"

#include <csetjmp>
#include <string>
#include <vector>

#include "base/integral_types.h"
#include "base/logging.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "lua/convert_value.h"
#include "lua/interpreter_impl.h"
#include "lua/table_local_object.h"
#include "third_party/lua-5.2.3/src/lobject.h"
#include "third_party/lua-5.2.3/src/lua.h"

using std::longjmp;
using std::vector;

namespace floating_temple {

class ObjectReference;

namespace lua {
namespace {

Thread* GetThreadObject() {
  return InterpreterImpl::instance()->GetThreadObject();
}

InterpreterImpl::LongJumpTarget* GetLongJumpTarget() {
  return InterpreterImpl::instance()->GetLongJumpTarget();
}

bool CallMethodHelper_GetTable(lua_State* lua_state, const TValue* table,
                               const TValue* key, StkId val) {
  ObjectReference* const table_object_reference = static_cast<ObjectReference*>(
      val_(table).ft_obj);

  vector<Value> parameters(1);
  LuaValueToValue(key, &parameters[0]);

  Value return_value;
  if (!GetThreadObject()->CallMethod(table_object_reference, "gettable",
                                     parameters, &return_value)) {
    return false;
  }

  ValueToLuaValue(lua_state, return_value, val);
  return true;
}

bool CallMethodHelper_SetTable(lua_State* lua_state, const TValue* table,
                               const TValue* key, const TValue* val) {
  ObjectReference* const table_object_reference = static_cast<ObjectReference*>(
      val_(table).ft_obj);

  vector<Value> parameters(2);
  LuaValueToValue(key, &parameters[0]);
  LuaValueToValue(val, &parameters[1]);

  Value return_value;
  if (!GetThreadObject()->CallMethod(table_object_reference, "settable",
                                     parameters, &return_value)) {
    return false;
  }

  CHECK_EQ(return_value.type(), Value::EMPTY);
  return true;
}

bool CallMethodHelper_ObjLen(lua_State* lua_state, StkId ra, const TValue* rb) {
  ObjectReference* const table_object_reference = static_cast<ObjectReference*>(
      val_(rb).ft_obj);

  Value return_value;
  if (!GetThreadObject()->CallMethod(table_object_reference, "len",
                                     vector<Value>(), &return_value)) {
    return false;
  }

  ValueToLuaValue(lua_state, return_value, ra);
  return true;
}

bool CallMethodHelper_SetList(lua_State* lua_state, const TValue* ra, int n,
                              int c) {
  ObjectReference* const table_object_reference = static_cast<ObjectReference*>(
      val_(ra).ft_obj);

  vector<Value> parameters(n + 1);
  parameters[0].set_int64_value(LUA_TNONE, static_cast<int64>(c));

  for (int i = n; i > 0; --i) {
    LuaValueToValue(ra + i, &parameters[i]);
  }

  Value return_value;
  if (!GetThreadObject()->CallMethod(table_object_reference, "setlist",
                                     parameters, &return_value)) {
    return false;
  }

  CHECK_EQ(return_value.type(), Value::EMPTY);
  return true;
}

}  // namespace

int AreObjectsEqual(const void* ft_obj1, const void* ft_obj2) {
  return GetThreadObject()->ObjectsAreIdentical(
      static_cast<const ObjectReference*>(ft_obj1),
      static_cast<const ObjectReference*>(ft_obj2)) ? 1 : 0;
}

int CreateTable(lua_State* lua_state, StkId obj, int b, int c) {
  VersionedLocalObject* const local_object = new TableLocalObject(lua_state, b,
                                                                  c);
  ObjectReference* const object_reference =
      GetThreadObject()->CreateVersionedObject(local_object, "");

  val_(obj).ft_obj = object_reference;
  settt_(obj, LUA_TTABLE);

  return 1;
}

// Since the following functions call longjmp(), they must not create any
// objects that have destructors.

int CallMethod_GetTable(lua_State* lua_state, const TValue* table,
                        const TValue* key, StkId val) {
  if (!ttisfloatingtemplateobject(table)) {
    return 0;
  }

  if (!CallMethodHelper_GetTable(lua_state, table, key, val)) {
    longjmp(GetLongJumpTarget()->env, 1);
  }

  return 1;
}

int CallMethod_SetTable(lua_State* lua_state, const TValue* table,
                        const TValue* key, const TValue* val) {
  if (!ttisfloatingtemplateobject(table)) {
    return 0;
  }

  if (!CallMethodHelper_SetTable(lua_state, table, key, val)) {
    longjmp(GetLongJumpTarget()->env, 1);
  }

  return 1;
}

int CallMethod_ObjLen(lua_State* lua_state, StkId ra, const TValue* rb) {
  if (!ttisfloatingtemplateobject(rb)) {
    return 0;
  }

  if (!CallMethodHelper_ObjLen(lua_state, ra, rb)) {
    longjmp(GetLongJumpTarget()->env, 1);
  }

  return 1;
}

int CallMethod_SetList(lua_State* lua_state, const TValue* ra, int n, int c) {
  if (!ttisfloatingtemplateobject(ra)) {
    return 0;
  }

  if (!CallMethodHelper_SetList(lua_state, ra, n, c)) {
    longjmp(GetLongJumpTarget()->env, 1);
  }

  return 1;
}

}  // namespace lua
}  // namespace floating_temple
