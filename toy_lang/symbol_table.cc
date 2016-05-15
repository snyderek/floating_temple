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

using std::string;
using std::unordered_map;

namespace floating_temple {
namespace toy_lang {

SymbolTable::SymbolTable()
    : next_symbol_id_(1) {
}

SymbolTable::~SymbolTable() {
}

void SymbolTable::EnterScope() {
  scopes_.emplace_back();
}

void SymbolTable::LeaveScope() {
  scopes_.pop_back();
}

int SymbolTable::GetSymbolId(const string& symbol_name) {
  CHECK(!scopes_.empty());
  CHECK(!symbol_name.empty());

  for (auto scope_it = scopes_.rbegin(); scope_it != scopes_.rend();
       ++scope_it) {
    const unordered_map<string, int>& symbol_map = *scope_it;

    const auto it = symbol_map.find(symbol_name);
    if (it != symbol_map.end()) {
      return it->second;
    }
  }

  const int symbol_id = next_symbol_id_;
  ++next_symbol_id_;
  CHECK(scopes_.back().emplace(symbol_name, symbol_id).second);
  return symbol_id;
}

}  // namespace toy_lang
}  // namespace floating_temple
