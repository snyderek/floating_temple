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

#include "toy_lang/zoo/symbol_table_object.h"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/escape.h"
#include "base/integral_types.h"
#include "base/linked_ptr.h"
#include "base/logging.h"
#include "base/mutex.h"
#include "base/mutex_lock.h"
#include "base/string_printf.h"
#include "include/c++/deserialization_context.h"
#include "include/c++/object_reference.h"
#include "include/c++/serialization_context.h"
#include "toy_lang/proto/serialization.pb.h"
#include "util/dump_context.h"

using std::pair;
using std::string;
using std::unordered_map;
using std::vector;

namespace floating_temple {
namespace toy_lang {

SymbolTableObject::SymbolTableObject()
    : scopes_(1,
              make_linked_ptr(new unordered_map<string, ObjectReference*>())) {
}

VersionedLocalObject* SymbolTableObject::Clone() const {
  SymbolTableObject* new_object = new SymbolTableObject();

  {
    MutexLock lock(&scopes_mu_);

    const ScopeVector::size_type symbol_table_size = scopes_.size();
    new_object->scopes_.resize(symbol_table_size);

    for (ScopeVector::size_type i = 0; i < symbol_table_size; ++i) {
      new_object->scopes_[i].reset(
          new unordered_map<string, ObjectReference*>(*scopes_[i]));
    }
  }

  return new_object;
}

void SymbolTableObject::InvokeMethod(Thread* thread,
                                     ObjectReference* object_reference,
                                     const string& method_name,
                                     const vector<Value>& parameters,
                                     Value* return_value) {
  CHECK(return_value != nullptr);

  if (VLOG_IS_ON(4)) {
    VLOG(4) << GetStringForLogging();
  }

  if (method_name == "enter_scope") {
    CHECK_EQ(parameters.size(), 0u);
    unordered_map<string, ObjectReference*>* const symbol_map =
        new unordered_map<string, ObjectReference*>();
    {
      MutexLock lock(&scopes_mu_);
      scopes_.emplace_back(symbol_map);
    }
    return_value->set_empty(0);
  } else if (method_name == "leave_scope") {
    CHECK(!scopes_.empty());
    CHECK_EQ(parameters.size(), 0u);
    {
      MutexLock lock(&scopes_mu_);
      scopes_.pop_back();
    }
    return_value->set_empty(0);
  } else if (method_name == "is_set") {
    CHECK_EQ(parameters.size(), 1u);

    const string& symbol_name = parameters[0].string_value();

    {
      MutexLock lock(&scopes_mu_);

      for (ScopeVector::const_reverse_iterator it = scopes_.rbegin();
           it != scopes_.rend(); ++it) {
        const unordered_map<string, ObjectReference*>& symbol_map = **it;

        if (symbol_map.find(symbol_name) != symbol_map.end()) {
          return_value->set_bool_value(0, true);
          return;
        }
      }
    }

    return_value->set_bool_value(0, false);
  } else if (method_name == "get") {
    CHECK_EQ(parameters.size(), 1u);

    const string& symbol_name = parameters[0].string_value();

    {
      MutexLock lock(&scopes_mu_);

      for (ScopeVector::const_reverse_iterator it = scopes_.rbegin();
           it != scopes_.rend(); ++it) {
        const unordered_map<string, ObjectReference*>& symbol_map = **it;

        const unordered_map<string, ObjectReference*>::const_iterator it2 =
            symbol_map.find(symbol_name);
        if (it2 != symbol_map.end()) {
          return_value->set_object_reference(0, it2->second);
          return;
        }
      }
    }

    LOG(FATAL) << "Symbol not found: \"" << CEscape(symbol_name) << "\"";
  } else if (method_name == "set") {
    CHECK_EQ(parameters.size(), 2u);

    const string& symbol_name = parameters[0].string_value();

    ObjectReference* const object_reference = parameters[1].object_reference();

    {
      MutexLock lock(&scopes_mu_);

      for (ScopeVector::reverse_iterator it = scopes_.rbegin();
           it != scopes_.rend(); ++it) {
        unordered_map<string, ObjectReference*>* const symbol_map = it->get();

        const unordered_map<string, ObjectReference*>::iterator it2 =
            symbol_map->find(symbol_name);
        if (it2 != symbol_map->end()) {
          it2->second = object_reference;
          return_value->set_empty(0);
          return;
        }
      }

      (*scopes_.back())[symbol_name] = object_reference;
    }

    return_value->set_empty(0);
  } else {
    LOG(FATAL) << "Unsupported method: \"" << CEscape(method_name) << "\"";
  }
}

void SymbolTableObject::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  ScopeVector scopes_temp;
  {
    MutexLock lock(&scopes_mu_);

    scopes_temp.reserve(scopes_.size());
    for (const auto& symbol_map : scopes_) {
      scopes_temp.emplace_back(
          new unordered_map<string, ObjectReference*>(*symbol_map));
    }
  }

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("SymbolTableObject");

  dc->AddString("scopes");
  dc->BeginList();
  for (const auto& symbol_map : scopes_temp) {
    dc->BeginMap();
    for (const pair<string, ObjectReference*>& map_pair : *symbol_map) {
      dc->AddString(map_pair.first);
      map_pair.second->Dump(dc);
    }
    dc->End();
  }
  dc->End();

  dc->End();
}

// static
SymbolTableObject* SymbolTableObject::ParseSymbolTableProto(
    const SymbolTableProto& symbol_table_proto,
    DeserializationContext* context) {
  CHECK(context != nullptr);

  SymbolTableObject* const new_object = new SymbolTableObject();
  ScopeVector* const scope = &new_object->scopes_;

  scope->resize(symbol_table_proto.map_size());

  for (int i = 0; i < symbol_table_proto.map_size(); ++i) {
    const SymbolMapProto& symbol_map_proto = symbol_table_proto.map(i);

    unordered_map<string, ObjectReference*>* const symbol_map =
        new unordered_map<string, ObjectReference*>();
    (*scope)[i].reset(symbol_map);

    for (int j = 0; j < symbol_map_proto.definition_size(); ++j) {
      const SymbolDefinitionProto& symbol_definition_proto =
          symbol_map_proto.definition(j);

      const string& name = symbol_definition_proto.name();
      const int64 object_index = symbol_definition_proto.object_index();
      ObjectReference* const object_reference =
          context->GetObjectReferenceByIndex(object_index);

      CHECK(symbol_map->emplace(name, object_reference).second);
    }
  }

  return new_object;
}

void SymbolTableObject::PopulateObjectProto(
    ObjectProto* object_proto, SerializationContext* context) const {
  CHECK(context != nullptr);

  SymbolTableProto* const symbol_table_proto =
      object_proto->mutable_symbol_table_object();

  MutexLock lock(&scopes_mu_);

  for (const auto& scope : scopes_) {
    const unordered_map<string, ObjectReference*>& symbol_map = *scope;
    SymbolMapProto* const symbol_map_proto = symbol_table_proto->add_map();

    for (const auto& symbol_pair : symbol_map) {
      SymbolDefinitionProto* const symbol_definition_proto =
          symbol_map_proto->add_definition();

      ObjectReference* const object_reference = symbol_pair.second;
      const int object_index = context->GetIndexForObjectReference(
          object_reference);

      symbol_definition_proto->set_name(symbol_pair.first);
      symbol_definition_proto->set_object_index(object_index);
    }
  }
}

string SymbolTableObject::GetStringForLogging() const {
  MutexLock lock(&scopes_mu_);

  string msg = "{";
  for (ScopeVector::const_iterator it = scopes_.begin(); it != scopes_.end();
       ++it) {
    if (it != scopes_.begin()) {
      msg += ",";
    }

    msg += " {";
    const unordered_map<string, ObjectReference*>& symbol_map = **it;
    for (unordered_map<string, ObjectReference*>::const_iterator it2 =
             symbol_map.begin();
         it2 != symbol_map.end(); ++it2) {
      if (it2 != symbol_map.begin()) {
        msg += ",";
      }

      StringAppendF(&msg, " \"%s\"", CEscape(it2->first).c_str());
    }
    msg += " }";
  }
  msg += " }";

  return msg;
}

}  // namespace toy_lang
}  // namespace floating_temple
