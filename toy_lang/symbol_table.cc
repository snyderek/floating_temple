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

#include "toy_lang/symbol_table.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/logging.h"
#include "toy_lang/symbol.h"

using std::string;
using std::unordered_map;

namespace floating_temple {
namespace toy_lang {

SymbolTable::SymbolTable() {
}

SymbolTable::~SymbolTable() {
}

void SymbolTable::EnterScope() {
  scopes_.emplace_back();
}

void SymbolTable::LeaveScope() {
  scopes_.pop_back();
}

const Symbol* SymbolTable::GetSymbol(const string& symbol_name) {
  CHECK(!scopes_.empty());
  CHECK(!symbol_name.empty());

  for (auto scope_it = scopes_.rbegin(); scope_it != scopes_.rend();
       ++scope_it) {
    const unordered_map<string, const Symbol*>& symbol_map = *scope_it;

    const auto it = symbol_map.find(symbol_name);
    if (it != symbol_map.end()) {
      return it->second;
    }
  }

  Symbol* const symbol = new Symbol(symbol_name);
  all_symbols_.emplace_back(symbol);
  CHECK(scopes_.back().emplace(symbol_name, symbol).second);
  return symbol;
}

}  // namespace toy_lang
}  // namespace floating_temple
