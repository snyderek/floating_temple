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

#include "lua/run_lua_program.h"

#include <string>

#include "base/logging.h"
#include "include/c++/peer.h"
#include "include/c++/value.h"
#include "lua/interpreter_impl.h"
#include "lua/program_object.h"
#include "lua/third_party_lua_headers.h"

using std::string;

namespace floating_temple {
namespace lua {

void RunLuaProgram(InterpreterImpl* interpreter, Peer* peer,
                   const string& source_file_name, bool linger) {
  CHECK(interpreter != nullptr);
  CHECK(peer != nullptr);

  lua_State* const lua_state = interpreter->GetLuaState();

  // This code is mostly copy-pasted from the 'main' and 'pmain' functions in
  // "third_party/lua-5.2.3/src/lua.c".

  // Tell libraries to ignore environment variables.
  lua_pushboolean(lua_state, 1);
  lua_setfield(lua_state, LUA_REGISTRYINDEX, "LUA_NOENV");

  // Check the version number of the Lua interpreter.
  luaL_checkversion(lua_state);

  // Open the Lua standard libraries. (Stop the garbage collector during
  // initialization.)
  lua_gc(lua_state, LUA_GCSTOP, 0);
  luaL_openlibs(lua_state);
  lua_gc(lua_state, LUA_GCRESTART, 0);

  // Load the Lua source file.
  CHECK_EQ(luaL_loadfile(lua_state, source_file_name.c_str()), LUA_OK);

  // TODO(dss): Pass the source file chunk as a parameter to the "run" method of
  // ProgramObject. This is kind of tricky because the chunk must be available
  // in every thread's lua_State.

  // Execute the source file inside the Floating Temple engine.
  Value return_value;
  peer->RunProgram(new ProgramObject(), "run", &return_value, linger);
  CHECK_EQ(return_value.type(), Value::EMPTY);
}

}  // namespace lua
}  // namespace floating_temple
