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
#include "lua/program_object.h"

using std::string;

namespace floating_temple {

class UnversionedLocalObject;

namespace lua {

int RunLuaProgram(Peer* peer, const string& source_file_name, bool linger) {
  CHECK(peer != nullptr);

  UnversionedLocalObject* const program_object = new ProgramObject(
      source_file_name);

  Value return_value;
  peer->RunProgram(program_object, "run", &return_value, linger);

  return static_cast<int>(return_value.int64_value());
}

}  // namespace lua
}  // namespace floating_temple
