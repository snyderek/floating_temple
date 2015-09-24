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
#include <unordered_map>
#include <vector>

#include "base/linked_ptr.h"
#include "base/macros.h"

namespace floating_temple {

class DumpContext;
class ObjectReference;

namespace toy_lang {

// This class is thread-compatible.
class SymbolTable {
 public:
  SymbolTable();

  void EnterScope();
  void LeaveScope();

  bool IsVariableSet(const std::string& name);
  ObjectReference* GetVariable(const std::string& name);
  void SetVariable(const std::string& name, ObjectReference* object);

  void Dump(DumpContext* dc) const;

 private:
  typedef std::vector<linked_ptr<std::unordered_map<std::string,
                                                    ObjectReference*>>>
      ScopeVector;

  ScopeVector scopes_;

  DISALLOW_COPY_AND_ASSIGN(SymbolTableObject);
};

}  // namespace toy_lang
}  // namespace floating_temple

#endif  // TOY_LANG_SYMBOL_TABLE_H_
