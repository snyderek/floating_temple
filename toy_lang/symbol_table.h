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
namespace toy_lang {

class SymbolTable {
 public:
  SymbolTable();
  ~SymbolTable();

  void EnterScope(const std::vector<std::string>& parameter_names);
  void LeaveScope(std::vector<int>* parameter_symbol_ids,
                  std::vector<int>* local_symbol_ids);

  int GetSymbolId(const std::string& symbol_name, bool visible) const;
  int GetLocalVariable(const std::string& symbol_name);
  int AddExternalSymbol(const std::string& symbol_name, bool visible);

  void GetExternalSymbolIds(
      std::unordered_map<std::string, int>* external_symbol_ids) const;

 private:
  struct Scope {
    std::unordered_map<std::string, int> symbol_map;
    std::vector<int> parameter_symbol_ids;
    std::vector<int> local_symbol_ids;
  };

  struct ExternalSymbol {
    int symbol_id;
    bool visible;
  };

  bool PrivateGetSymbolId(const std::string& symbol_name, bool visible,
                          int* symbol_id) const;
  int CreateSymbol(const std::string& symbol_name);
  int GetNextSymbolId();

  std::vector<Scope> scopes_;
  std::unordered_map<std::string, ExternalSymbol> external_symbol_map_;
  int next_symbol_id_;

  DISALLOW_COPY_AND_ASSIGN(SymbolTable);
};

}  // namespace toy_lang
}  // namespace floating_temple

#endif  // TOY_LANG_SYMBOL_TABLE_H_
