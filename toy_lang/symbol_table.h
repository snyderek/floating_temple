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

#ifndef TOY_LANG_SYMBOL_TABLE_H_
#define TOY_LANG_SYMBOL_TABLE_H_

#include <string>

namespace floating_temple {

class PeerObject;
class Thread;

namespace toy_lang {

bool EnterScope(PeerObject* symbol_table_object, Thread* thread);
bool LeaveScope(PeerObject* symbol_table_object, Thread* thread);

bool IsVariableSet(PeerObject* symbol_table_object, Thread* thread,
                   const std::string& name, bool* is_set);
bool GetVariable(PeerObject* symbol_table_object, Thread* thread,
                 const std::string& name, PeerObject** object);
bool SetVariable(PeerObject* symbol_table_object, Thread* thread,
                 const std::string& name, PeerObject* object);

}  // namespace toy_lang
}  // namespace floating_temple

#endif  // TOY_LANG_SYMBOL_TABLE_H_
