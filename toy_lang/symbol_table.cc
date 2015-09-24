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

#include <string>
#include <utility>
#include <vector>

#include "base/escape.h"
#include "base/logging.h"
#include "include/c++/object_reference.h"
#include "util/dump_context.h"

using std::pair;
using std::string;
using std::unordered_map;
using std::vector;

namespace floating_temple {
namespace toy_lang {

void SymbolTable::EnterScope() {
  scopes_.emplace_back(new unordered_map<string, ObjectReference*>());
}

void SymbolTable::LeaveScope() {
  CHECK(!scopes_.empty());
  scopes_.pop_back();
}

bool SymbolTable::IsVariableSet(const string& name) {
  VLOG(1) << "Symbol table: Is set \"" << CEscape(name) << "\"";

  for (ScopeVector::const_reverse_iterator it = scopes_.rbegin();
       it != scopes_.rend(); ++it) {
    const unordered_map<string, ObjectReference*>& symbol_map = **it;

    if (symbol_map.find(name) != symbol_map.end()) {
      return true;
    }
  }

  return false;
}

ObjectReference* SymbolTable::GetVariable(const string& name) {
  VLOG(1) << "Symbol table: Get \"" << CEscape(name) << "\"";

  for (ScopeVector::const_reverse_iterator it = scopes_.rbegin();
       it != scopes_.rend(); ++it) {
    const unordered_map<string, ObjectReference*>& symbol_map = **it;

    const unordered_map<string, ObjectReference*>::const_iterator it2 =
        symbol_map.find(name);
    if (it2 != symbol_map.end()) {
      return it2->second;
    }
  }

  LOG(FATAL) << "Symbol not found: \"" << CEscape(name) << "\"";
}

void SymbolTable::SetVariable(const string& name, ObjectReference* object) {
  VLOG(1) << "Symbol table: Set \"" << CEscape(name) << "\"";

  for (ScopeVector::reverse_iterator it = scopes_.rbegin();
       it != scopes_.rend(); ++it) {
    unordered_map<string, ObjectReference*>* const symbol_map = it->get();

    const unordered_map<string, ObjectReference*>::iterator it2 =
        symbol_map->find(name);
    if (it2 != symbol_map->end()) {
      it2->second = object;
      return;
    }
  }

  (*scopes_.back())[name] = object;
}

void SymbolTable::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginList();
  for (const auto& symbol_map : scopes_) {
    dc->BeginMap();
    for (const pair<string, ObjectReference*>& map_pair : *symbol_map) {
      dc->AddString(map_pair.first);
      map_pair.second->Dump(dc);
    }
    dc->End();
  }
  dc->End();
}

}  // namespace toy_lang
}  // namespace floating_temple
