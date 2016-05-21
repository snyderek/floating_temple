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

#include "base/escape.h"
#include "base/logging.h"

using std::string;
using std::unordered_map;
using std::vector;

namespace floating_temple {
namespace toy_lang {

SymbolTable::SymbolTable()
    : next_symbol_id_(1) {
}

SymbolTable::~SymbolTable() {
}

void SymbolTable::EnterScope(const vector<string>& parameter_names) {
  scopes_.emplace_back();

  vector<int>* const parameter_symbol_ids =
      &scopes_.back().parameter_symbol_ids;
  parameter_symbol_ids->reserve(parameter_names.size());

  for (const string& parameter_name : parameter_names) {
    const int symbol_id = CreateSymbol(parameter_name);
    parameter_symbol_ids->push_back(symbol_id);
  }
}

void SymbolTable::LeaveScope(vector<int>* parameter_symbol_ids,
                             vector<int>* local_symbol_ids) {
  CHECK(!scopes_.empty());
  CHECK(parameter_symbol_ids != nullptr);
  CHECK(local_symbol_ids != nullptr);

  parameter_symbol_ids->swap(scopes_.back().parameter_symbol_ids);
  local_symbol_ids->swap(scopes_.back().local_symbol_ids);

  scopes_.pop_back();
}

int SymbolTable::GetSymbolId(const string& symbol_name) const {
  for (auto scope_it = scopes_.rbegin(); scope_it != scopes_.rend();
       ++scope_it) {
    const unordered_map<string, int>& symbol_map = scope_it->symbol_map;

    const auto it = symbol_map.find(symbol_name);
    if (it != symbol_map.end()) {
      return it->second;
    }
  }

  const auto it = external_symbol_map_.find(symbol_name);
  if (it != external_symbol_map_.end()) {
    return it->second;
  }

  LOG(FATAL) << "Symbol not found: \"" << CEscape(symbol_name) << "\"";
}

int SymbolTable::CreateLocalVariable(const string& symbol_name) {
  const int symbol_id = CreateSymbol(symbol_name);
  scopes_.back().local_symbol_ids.push_back(symbol_id);
  return symbol_id;
}

int SymbolTable::AddExternalSymbol(const string& symbol_name) {
  const int symbol_id = GetNextSymbolId();
  if (!symbol_name.empty()) {
    CHECK(external_symbol_map_.emplace(symbol_name, symbol_id).second);
  }

  return symbol_id;
}

int SymbolTable::CreateSymbol(const string& symbol_name) {
  CHECK(!scopes_.empty());
  CHECK(!symbol_name.empty());

  const int symbol_id = GetNextSymbolId();
  CHECK(scopes_.back().symbol_map.emplace(symbol_name, symbol_id).second);

  return symbol_id;
}

int SymbolTable::GetNextSymbolId() {
  return next_symbol_id_++;
}

}  // namespace toy_lang
}  // namespace floating_temple
