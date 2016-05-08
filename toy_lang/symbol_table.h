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

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/macros.h"

namespace floating_temple {

class ObjectReference;
class Thread;

namespace toy_lang {

class Symbol;

class SymbolTable {
 public:
  SymbolTable();
  ~SymbolTable();

  void EnterScope();
  void LeaveScope();

  const Symbol* GetSymbol(const std::string& symbol_name);

 private:
  std::vector<std::unique_ptr<Symbol>> all_symbols_;
  std::vector<std::unordered_map<std::string, const Symbol*>> scopes_;

  DISALLOW_COPY_AND_ASSIGN(SymbolTable);
};

// TODO(dss): Delete all of the following functions.

bool EnterScope(ObjectReference* symbol_table_object, Thread* thread);
bool LeaveScope(ObjectReference* symbol_table_object, Thread* thread);

bool IsVariableSet(ObjectReference* symbol_table_object, Thread* thread,
                   const std::string& name, bool* is_set);
bool GetVariable(ObjectReference* symbol_table_object, Thread* thread,
                 const std::string& name, ObjectReference** object);
bool SetVariable(ObjectReference* symbol_table_object, Thread* thread,
                 const std::string& name, ObjectReference* object);

}  // namespace toy_lang
}  // namespace floating_temple

#endif  // TOY_LANG_SYMBOL_TABLE_H_
