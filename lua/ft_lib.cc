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

#include "lua/ft_lib.h"

#include <string>

#include "base/logging.h"
#include "include/c++/thread.h"
#include "lua/interpreter_impl.h"
#include "lua/table_local_object.h"
#include "lua/third_party_lua_headers.h"

namespace floating_temple {

class ObjectReference;

namespace lua {
namespace {

int BeginTransaction(lua_State* lua_state) {
  InterpreterImpl::instance()->BeginTransaction();
  return 0;
}

int EndTransaction(lua_State* lua_state) {
  InterpreterImpl::instance()->EndTransaction();
  return 0;
}

const luaL_Reg ft_funcs[] = {
  { "begin_tran", &BeginTransaction },
  { "end_tran", &EndTransaction },
  { nullptr, nullptr }
};

LUAMOD_API int OpenFloatingTempleLib(lua_State* lua_state) {
  CHECK(lua_state != nullptr);

  InterpreterImpl* const interpreter = InterpreterImpl::instance();

  // Create the floating_temple library, and register the library functions.
  luaL_newlib(lua_state, ft_funcs);

  // Create the "shared" table, which will be shared with remote peers.
  TableLocalObject* const local_object = new TableLocalObject(interpreter);
  local_object->Init(0, 0);
  ObjectReference* const object_reference =
      interpreter->GetThreadObject()->CreateVersionedObject(local_object,
                                                            "shared");

  // Push a reference to the shared table onto the stack.
  lua_lock(lua_state);
  val_(lua_state->top).ft_obj = object_reference;
  settt_(lua_state->top, LUA_TFLOATINGTEMPLEOBJECT);
  api_incr_top(lua_state);
  lua_unlock(lua_state);

  // Within the floating_temple library, set the field name "shared" to point to
  // the shared table.
  //
  // The reference to the library is at stack index top-2, and the reference to
  // the shared table is at top-1. lua_setfield will pop the table reference off
  // the stack.
  lua_setfield(lua_state, -2, "shared");

  // Return with the reference to the newly created floating_temple library
  // still on the stack.
  return 1;
}

}  // namespace

void InstallFloatingTempleLib(lua_State* lua_state) {
  luaL_requiref(lua_state, "floating_temple", &OpenFloatingTempleLib, 1);

  // Pop the reference to the floating_temple library off the stack.
  lua_pop(lua_state, 1);
}

}  // namespace lua
}  // namespace floating_temple
