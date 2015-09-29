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

#ifndef LUA_RUN_LUA_PROGRAM_H_
#define LUA_RUN_LUA_PROGRAM_H_

#include <string>

namespace floating_temple {

class Peer;

namespace lua {

class InterpreterImpl;

void RunLuaProgram(InterpreterImpl* interpreter, Peer* peer,
                   const std::string& source_file_name, bool linger);

}  // namespace lua
}  // namespace floating_temple

#endif  // LUA_RUN_LUA_PROGRAM_H_