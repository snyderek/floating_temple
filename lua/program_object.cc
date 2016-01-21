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

#include "lua/program_object.h"

#include <cstdlib>
#include <string>
#include <vector>

#include "base/logging.h"
#include "include/c++/value.h"
#include "lua/ft_lib.h"
#include "lua/global_lock.h"
#include "lua/interpreter_impl.h"
#include "lua/third_party_lua_headers.h"
#include "lua/thread_substitution.h"
#include "util/dump_context.h"

using std::string;
using std::vector;

namespace floating_temple {
namespace lua {

ProgramObject::ProgramObject(const string& file_name,
                             const string& file_content)
    : file_name_(file_name),
      file_content_(file_content) {
}

void ProgramObject::InvokeMethod(Thread* thread,
                                 ObjectReference* object_reference,
                                 const string& method_name,
                                 const vector<Value>& parameters,
                                 Value* return_value) {
  CHECK_EQ(method_name, "run");
  CHECK(return_value != nullptr);

  InterpreterImpl* const interpreter = InterpreterImpl::instance();

  ThreadSubstitution thread_substitution(interpreter, thread);

  lua_State* const lua_state = luaL_newstate();
  CHECK(lua_state != nullptr);

  {
    GlobalLock global_lock(interpreter);

    // Load the standard Lua libraries. (Temporarily suspend garbage collection
    // while the libraries are being loaded.)
    lua_gc(lua_state, LUA_GCSTOP, 0);
    // TODO(dss): Tell the standard libraries to ignore environment variables.
    luaL_openlibs(lua_state);
    InstallFloatingTempleLib(lua_state);
    lua_gc(lua_state, LUA_GCRESTART, 0);

    // Load the content of the source file into the Lua interpreter.
    CHECK_EQ(luaL_loadbuffer(lua_state, file_content_.data(),
                             file_content_.length(), file_name_.c_str()),
             LUA_OK);

    // Run the Lua program.
    CHECK_EQ(lua_pcall(lua_state, 0, 0, 0), LUA_OK)
        << lua_tostring(lua_state, -1);

    lua_close(lua_state);
  }

  // TODO(dss): Print any error message reported by the Lua interpreter.

  // TODO(dss): Return EXIT_FAILURE if an error occurred.
  return_value->set_int64_value(0, EXIT_SUCCESS);
}

void ProgramObject::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();
  dc->AddString("type");
  dc->AddString("ProgramObject");
  dc->End();
}

}  // namespace lua
}  // namespace floating_temple
