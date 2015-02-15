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

#include "toy_lang/local_object_impl.h"

#include <cstddef>
#include <cstdio>
#include <string>
#include <tr1/cinttypes>
#include <tr1/unordered_map>
#include <utility>
#include <vector>

#include "base/const_shared_ptr.h"
#include "base/escape.h"
#include "base/integral_types.h"
#include "base/linked_ptr.h"
#include "base/logging.h"
#include "base/mutex.h"
#include "base/mutex_lock.h"
#include "base/string_printf.h"
#include "include/c++/deserialization_context.h"
#include "include/c++/peer_object.h"
#include "include/c++/serialization_context.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "toy_lang/expression.h"
#include "toy_lang/get_serialized_object_type.h"
#include "toy_lang/program_object.h"
#include "toy_lang/proto/serialization.pb.h"
#include "toy_lang/symbol_table.h"

using std::make_pair;
using std::printf;
using std::size_t;
using std::string;
using std::tr1::unordered_map;
using std::vector;

namespace floating_temple {
namespace toy_lang {
namespace {

int64 TrueMod(int64 a, int64 b) {
  CHECK_NE(b, 0);
  return (a % b + b) % b;
}

}  // namespace

size_t LocalObjectImpl::Serialize(void* buffer, size_t buffer_size,
                                  SerializationContext* context) const {
  ObjectProto object_proto;
  // TODO(dss): Use the 'this' keyword when calling a pure virtual method on
  // this object.
  PopulateObjectProto(&object_proto, context);

  const size_t byte_size = static_cast<size_t>(object_proto.ByteSize());
  if (byte_size <= buffer_size) {
    object_proto.SerializeWithCachedSizesToArray(static_cast<uint8*>(buffer));
  }

  return byte_size;
}

// static
LocalObjectImpl* LocalObjectImpl::Deserialize(const void* buffer,
                                              size_t buffer_size,
                                              DeserializationContext* context) {
  CHECK(buffer != NULL);

  ObjectProto object_proto;
  CHECK(object_proto.ParseFromArray(buffer, buffer_size));

  const ObjectProto::Type object_type = GetSerializedObjectType(object_proto);

  switch (object_type) {
    case ObjectProto::NONE:
      return new NoneObject();

    case ObjectProto::BOOL:
      return BoolObject::ParseBoolProto(object_proto.bool_object());

    case ObjectProto::INT:
      return IntObject::ParseIntProto(object_proto.int_object());

    case ObjectProto::STRING:
      return StringObject::ParseStringProto(object_proto.string_object());

    case ObjectProto::SYMBOL_TABLE:
      return SymbolTableObject::ParseSymbolTableProto(
          object_proto.symbol_table_object(), context);

    case ObjectProto::EXPRESSION:
      return ExpressionObject::ParseExpressionProto(
          object_proto.expression_object());

    case ObjectProto::LIST:
      return ListObject::ParseListProto(object_proto.list_object(), context);

    case ObjectProto::MAP:
      return MapObject::ParseMapProto(object_proto.map_object(), context);

    case ObjectProto::RANGE_ITERATOR:
      return RangeIteratorObject::ParseRangeIteratorProto(
          object_proto.range_iterator_object());

    case ObjectProto::LIST_FUNCTION:
      return new ListFunction();

    case ObjectProto::SET_VARIABLE_FUNCTION:
      return new SetVariableFunction();

    case ObjectProto::FOR_FUNCTION:
      return new ForFunction();

    case ObjectProto::RANGE_FUNCTION:
      return new RangeFunction();

    case ObjectProto::PRINT_FUNCTION:
      return new PrintFunction();

    case ObjectProto::ADD_FUNCTION:
      return new AddFunction();

    case ObjectProto::BEGIN_TRAN_FUNCTION:
      return new BeginTranFunction();

    case ObjectProto::END_TRAN_FUNCTION:
      return new EndTranFunction();

    case ObjectProto::IF_FUNCTION:
      return new IfFunction();

    case ObjectProto::NOT_FUNCTION:
      return new NotFunction();

    case ObjectProto::IS_SET_FUNCTION:
      return new IsSetFunction();

    case ObjectProto::WHILE_FUNCTION:
      return new WhileFunction();

    case ObjectProto::LESS_THAN_FUNCTION:
      return new LessThanFunction();

    case ObjectProto::LEN_FUNCTION:
      return new LenFunction();

    case ObjectProto::APPEND_FUNCTION:
      return new AppendFunction();

    case ObjectProto::GET_AT_FUNCTION:
      return new GetAtFunction();

    case ObjectProto::MAP_IS_SET_FUNCTION:
      return new MapIsSetFunction();

    case ObjectProto::MAP_GET_FUNCTION:
      return new MapGetFunction();

    case ObjectProto::MAP_SET_FUNCTION:
      return new MapSetFunction();

    case ObjectProto::PROGRAM: {
      Expression* const expression = Expression::ParseExpressionProto(
          object_proto.program_object().expression());
      return new ProgramObject(make_const_shared_ptr(expression));
    }

    default:
      LOG(FATAL) << "Unexpected object type: " << static_cast<int>(object_type);
  }

  return NULL;
}

NoneObject::NoneObject() {
}

LocalObject* NoneObject::Clone() const {
  return new NoneObject();
}

void NoneObject::InvokeMethod(Thread* thread,
                              PeerObject* peer_object,
                              const string& method_name,
                              const vector<Value>& parameters,
                              Value* return_value) {
  LOG(FATAL) << "Unsupported method: \"" << CEscape(method_name) << "\"";
}

string NoneObject::Dump() const {
  return "{ \"type\": \"NoneObject\" }";
}

void NoneObject::PopulateObjectProto(ObjectProto* object_proto,
                                     SerializationContext* context) const {
  object_proto->mutable_none_object();
}

BoolObject::BoolObject(bool b)
    : b_(b) {
}

LocalObject* BoolObject::Clone() const {
  return new BoolObject(b_);
}

void BoolObject::InvokeMethod(Thread* thread,
                              PeerObject* peer_object,
                              const string& method_name,
                              const vector<Value>& parameters,
                              Value* return_value) {
  CHECK(return_value != NULL);

  if (method_name == "get_bool") {
    CHECK_EQ(parameters.size(), 0u);
    return_value->set_bool_value(0, b_);
  } else if (method_name == "get_string") {
    CHECK_EQ(parameters.size(), 0u);
    if (b_) {
      return_value->set_string_value(0, "true");
    } else {
      return_value->set_string_value(0, "false");
    }
  } else {
    LOG(FATAL) << "Unsupported method: \"" << CEscape(method_name) << "\"";
  }
}

string BoolObject::Dump() const {
  return StringPrintf("{ \"type\": \"BoolObject\", \"b\": %s }",
                      b_ ? "true" : "false");
}

// static
BoolObject* BoolObject::ParseBoolProto(const BoolProto& bool_proto) {
  return new BoolObject(bool_proto.value());
}

void BoolObject::PopulateObjectProto(ObjectProto* object_proto,
                                     SerializationContext* context) const {
  object_proto->mutable_bool_object()->set_value(b_);
}

IntObject::IntObject(int64 n)
    : n_(n) {
}

LocalObject* IntObject::Clone() const {
  return new IntObject(n_);
}

void IntObject::InvokeMethod(Thread* thread,
                             PeerObject* peer_object,
                             const string& method_name,
                             const vector<Value>& parameters,
                             Value* return_value) {
  CHECK(return_value != NULL);

  if (method_name == "get_int") {
    CHECK_EQ(parameters.size(), 0u);
    return_value->set_int64_value(0, n_);
  } else if (method_name == "get_string") {
    CHECK_EQ(parameters.size(), 0u);
    return_value->set_string_value(0, StringPrintf("%" PRId64, n_));
  } else {
    LOG(FATAL) << "Unsupported method: \"" << CEscape(method_name) << "\"";
  }
}

string IntObject::Dump() const {
  return StringPrintf("{ \"type\": \"IntObject\", \"n\": %" PRId64 " }", n_);
}

// static
IntObject* IntObject::ParseIntProto(const IntProto& int_proto) {
  return new IntObject(int_proto.value());
}

void IntObject::PopulateObjectProto(ObjectProto* object_proto,
                                    SerializationContext* context) const {
  object_proto->mutable_int_object()->set_value(n_);
}

StringObject::StringObject(const string& s)
    : s_(s) {
}

LocalObject* StringObject::Clone() const {
  return new StringObject(s_);
}

void StringObject::InvokeMethod(Thread* thread,
                                PeerObject* peer_object,
                                const string& method_name,
                                const vector<Value>& parameters,
                                Value* return_value) {
  CHECK(return_value != NULL);

  if (method_name == "get_string") {
    CHECK_EQ(parameters.size(), 0u);
    return_value->set_string_value(0, s_);
  } else {
    LOG(FATAL) << "Unsupported method: \"" << CEscape(method_name) << "\"";
  }
}

string StringObject::Dump() const {
  return StringPrintf("{ \"type\": \"StringObject\", \"s\": \"%s\" }",
                      CEscape(s_.c_str()).c_str());
}

// static
StringObject* StringObject::ParseStringProto(const StringProto& string_proto) {
  return new StringObject(string_proto.value());
}

void StringObject::PopulateObjectProto(ObjectProto* object_proto,
                                       SerializationContext* context) const {
  object_proto->mutable_string_object()->set_value(s_);
}

SymbolTableObject::SymbolTableObject()
    : scopes_(1, make_linked_ptr(new unordered_map<string, PeerObject*>())) {
}

LocalObject* SymbolTableObject::Clone() const {
  SymbolTableObject* new_object = new SymbolTableObject();

  {
    MutexLock lock(&scopes_mu_);

    const ScopeVector::size_type symbol_table_size = scopes_.size();
    new_object->scopes_.resize(symbol_table_size);

    for (ScopeVector::size_type i = 0; i < symbol_table_size; ++i) {
      new_object->scopes_[i].reset(
          new unordered_map<string, PeerObject*>(*scopes_[i]));
    }
  }

  return new_object;
}

void SymbolTableObject::InvokeMethod(Thread* thread,
                                     PeerObject* peer_object,
                                     const string& method_name,
                                     const vector<Value>& parameters,
                                     Value* return_value) {
  CHECK(return_value != NULL);

  if (VLOG_IS_ON(4)) {
    VLOG(4) << GetStringForLogging();
  }

  if (method_name == "enter_scope") {
    CHECK_EQ(parameters.size(), 0u);
    unordered_map<string, PeerObject*>* const symbol_map =
        new unordered_map<string, PeerObject*>();
    {
      MutexLock lock(&scopes_mu_);
      scopes_.push_back(make_linked_ptr(symbol_map));
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
        const unordered_map<string, PeerObject*>& symbol_map = **it;

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
        const unordered_map<string, PeerObject*>& symbol_map = **it;

        const unordered_map<string, PeerObject*>::const_iterator it2 =
            symbol_map.find(symbol_name);
        if (it2 != symbol_map.end()) {
          return_value->set_peer_object(0, it2->second);
          return;
        }
      }
    }

    LOG(FATAL) << "Symbol not found: \"" << CEscape(symbol_name) << "\"";
  } else if (method_name == "set") {
    CHECK_EQ(parameters.size(), 2u);

    const string& symbol_name = parameters[0].string_value();

    PeerObject* const peer_object = parameters[1].peer_object();

    {
      MutexLock lock(&scopes_mu_);

      for (ScopeVector::reverse_iterator it = scopes_.rbegin();
           it != scopes_.rend(); ++it) {
        unordered_map<string, PeerObject*>* const symbol_map = it->get();

        const unordered_map<string, PeerObject*>::iterator it2 =
            symbol_map->find(symbol_name);
        if (it2 != symbol_map->end()) {
          it2->second = peer_object;
          return_value->set_empty(0);
          return;
        }
      }

      (*scopes_.back())[symbol_name] = peer_object;
    }

    return_value->set_empty(0);
  } else {
    LOG(FATAL) << "Unsupported method: \"" << CEscape(method_name) << "\"";
  }
}

string SymbolTableObject::Dump() const {
  string scopes_string;
  {
    MutexLock lock(&scopes_mu_);

    if (scopes_.empty()) {
      scopes_string = "[]";
    } else {
      scopes_string = "[";

      for (ScopeVector::const_iterator it = scopes_.begin();
           it != scopes_.end(); ++it) {
        if (it != scopes_.begin()) {
          scopes_string += ",";
        }

        const unordered_map<string, PeerObject*>& symbol_map = **it;

        if (symbol_map.empty()) {
          scopes_string += " {}";
        } else {
          scopes_string += " {";
          for (unordered_map<string, PeerObject*>::const_iterator it2 =
                   symbol_map.begin();
               it2 != symbol_map.end(); ++it2) {
            if (it2 != symbol_map.begin()) {
              scopes_string += ",";
            }

            StringAppendF(&scopes_string, " \"%s\": %s",
                          CEscape(it2->first).c_str(),
                          it2->second->Dump().c_str());
          }
          scopes_string += " }";
        }
      }
      scopes_string += " ]";
    }
  }

  return StringPrintf("{ \"type\": \"SymbolTableObject\", \"scopes\": %s }",
                      scopes_string.c_str());
}

// static
SymbolTableObject* SymbolTableObject::ParseSymbolTableProto(
    const SymbolTableProto& symbol_table_proto,
    DeserializationContext* context) {
  CHECK(context != NULL);

  SymbolTableObject* const new_object = new SymbolTableObject();
  ScopeVector* const scope = &new_object->scopes_;

  scope->resize(symbol_table_proto.map_size());

  for (int i = 0; i < symbol_table_proto.map_size(); ++i) {
    const SymbolMapProto& symbol_map_proto = symbol_table_proto.map(i);

    unordered_map<string, PeerObject*>* const symbol_map =
        new unordered_map<string, PeerObject*>();
    (*scope)[i].reset(symbol_map);

    for (int j = 0; j < symbol_map_proto.definition_size(); ++j) {
      const SymbolDefinitionProto& symbol_definition_proto =
          symbol_map_proto.definition(j);

      const string& name = symbol_definition_proto.name();
      const int64 object_index = symbol_definition_proto.object_index();
      PeerObject* const peer_object = context->GetPeerObjectByIndex(
          object_index);

      CHECK(symbol_map->insert(make_pair(name, peer_object)).second);
    }
  }

  return new_object;
}

void SymbolTableObject::PopulateObjectProto(
    ObjectProto* object_proto, SerializationContext* context) const {
  CHECK(context != NULL);

  SymbolTableProto* const symbol_table_proto =
      object_proto->mutable_symbol_table_object();

  MutexLock lock(&scopes_mu_);

  for (ScopeVector::const_iterator it = scopes_.begin(); it != scopes_.end();
       ++it) {
    const unordered_map<string, PeerObject*>& symbol_map = **it;
    SymbolMapProto* const symbol_map_proto = symbol_table_proto->add_map();

    for (unordered_map<string, PeerObject*>::const_iterator it2 =
             symbol_map.begin();
         it2 != symbol_map.end(); ++it2) {
      SymbolDefinitionProto* const symbol_definition_proto =
          symbol_map_proto->add_definition();

      PeerObject* const peer_object = it2->second;
      const int object_index = context->GetIndexForPeerObject(peer_object);

      symbol_definition_proto->set_name(it2->first);
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
    const unordered_map<string, PeerObject*>& symbol_map = **it;
    for (unordered_map<string, PeerObject*>::const_iterator it2 =
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

ExpressionObject::ExpressionObject(
    const const_shared_ptr<Expression>& expression)
    : expression_(expression) {
  CHECK(expression.get() != NULL);
}

LocalObject* ExpressionObject::Clone() const {
  return new ExpressionObject(expression_);
}

void ExpressionObject::InvokeMethod(Thread* thread,
                                    PeerObject* peer_object,
                                    const string& method_name,
                                    const vector<Value>& parameters,
                                    Value* return_value) {
  CHECK(return_value != NULL);

  if (method_name == "eval") {
    CHECK_EQ(parameters.size(), 1u);

    PeerObject* const symbol_table_object = parameters[0].peer_object();

    PeerObject* const peer_object = expression_->Evaluate(symbol_table_object,
                                                          thread);
    if (peer_object == NULL) {
      return;
    }
    return_value->set_peer_object(0, peer_object);
  } else {
    LOG(FATAL) << "Unsupported method: \"" << CEscape(method_name) << "\"";
  }
}

string ExpressionObject::Dump() const {
  // TODO(dss): Dump the contents of the expression.
  return "{ \"type\": \"ExpressionObject\" }";
}

// static
ExpressionObject* ExpressionObject::ParseExpressionProto(
    const ExpressionProto& expression_proto) {
  Expression* const expression = Expression::ParseExpressionProto(
      expression_proto);
  return new ExpressionObject(make_const_shared_ptr(expression));
}

void ExpressionObject::PopulateObjectProto(
    ObjectProto* object_proto, SerializationContext* context) const {
  expression_->PopulateExpressionProto(
      object_proto->mutable_expression_object());
}

ListObject::ListObject(const vector<PeerObject*>& items)
    : items_(items) {
}

LocalObject* ListObject::Clone() const {
  MutexLock lock(&items_mu_);
  return new ListObject(items_);
}

void ListObject::InvokeMethod(Thread* thread,
                              PeerObject* peer_object,
                              const string& method_name,
                              const vector<Value>& parameters,
                              Value* return_value) {
  CHECK(thread != NULL);
  CHECK(return_value != NULL);

  if (method_name == "length") {
    CHECK_EQ(parameters.size(), 0u);

    MutexLock lock(&items_mu_);
    return_value->set_int64_value(0, items_.size());
  } else if (method_name == "get_at") {
    CHECK_EQ(parameters.size(), 1u);

    const int64 index = parameters[0].int64_value();

    MutexLock lock(&items_mu_);
    CHECK(!items_.empty());
    const int64 length = static_cast<int64>(items_.size());
    return_value->set_peer_object(0, items_[TrueMod(index, length)]);
  } else if (method_name == "append") {
    CHECK_EQ(parameters.size(), 1u);

    {
      MutexLock lock(&items_mu_);
      items_.push_back(parameters[0].peer_object());
    }

    return_value->set_empty(0);
  } else if (method_name == "get_string") {
    CHECK_EQ(parameters.size(), 0u);

    string s = "[";
    {
      MutexLock lock(&items_mu_);

      for (vector<PeerObject*>::const_iterator it = items_.begin();
           it != items_.end(); ++it) {
        if (it != items_.begin()) {
          s += ' ';
        }

        Value item_str;
        if (!thread->CallMethod(*it, "get_string", vector<Value>(),
                                &item_str)) {
          return;
        }

        s += item_str.string_value();
      }
    }
    s += ']';

    return_value->set_string_value(0, s);
  } else {
    LOG(FATAL) << "Unsupported method: \"" << CEscape(method_name) << "\"";
  }
}

string ListObject::Dump() const {
  string items_string;
  {
    MutexLock lock(&items_mu_);

    if (items_.empty()) {
      items_string = "[]";
    } else {
      items_string = "[";
      for (vector<PeerObject*>::const_iterator it = items_.begin();
           it != items_.end(); ++it) {
        // TODO(dss): Insert commas between the list items.
        StringAppendF(&items_string, " %s", (*it)->Dump().c_str());
      }
      items_string += " ]";
    }
  }

  return StringPrintf("{ \"type\": \"ListObject\", \"items\": %s }",
                      items_string.c_str());
}

// static
ListObject* ListObject::ParseListProto(const ListProto& list_proto,
                                       DeserializationContext* context) {
  CHECK(context != NULL);

  vector<PeerObject*> items(list_proto.object_index_size());

  for (int i = 0; i < list_proto.object_index_size(); ++i) {
    const int64 object_index = list_proto.object_index(i);
    items[i] = context->GetPeerObjectByIndex(object_index);
  }

  return new ListObject(items);
}

void ListObject::PopulateObjectProto(ObjectProto* object_proto,
                                     SerializationContext* context) const {
  ListProto* const list_proto = object_proto->mutable_list_object();

  MutexLock lock(&items_mu_);

  for (vector<PeerObject*>::const_iterator it = items_.begin();
       it != items_.end(); ++it) {
    const int object_index = context->GetIndexForPeerObject(*it);
    list_proto->add_object_index(object_index);
  }
}

MapObject::MapObject() {
}

LocalObject* MapObject::Clone() const {
  MapObject* new_object = new MapObject();
  new_object->map_ = map_;

  return new_object;
}

void MapObject::InvokeMethod(Thread* thread,
                             PeerObject* peer_object,
                             const string& method_name,
                             const vector<Value>& parameters,
                             Value* return_value) {
  CHECK(return_value != NULL);

  if (method_name == "is_set") {
    CHECK_EQ(parameters.size(), 1u);

    const string& key = parameters[0].string_value();
    return_value->set_bool_value(0, map_.find(key) != map_.end());
  } else if (method_name == "get") {
    CHECK_EQ(parameters.size(), 1u);

    const string& key = parameters[0].string_value();

    const unordered_map<string, PeerObject*>::const_iterator it = map_.find(
        key);
    CHECK(it != map_.end()) << "Key not found: \"" << CEscape(key) << "\"";

    return_value->set_peer_object(0, it->second);
  } else if (method_name == "set") {
    CHECK_EQ(parameters.size(), 2u);

    const string& key = parameters[0].string_value();
    PeerObject* const peer_object = parameters[1].peer_object();

    map_[key] = peer_object;

    return_value->set_empty(0);
  } else {
    LOG(FATAL) << "Unsupported method: \"" << CEscape(method_name) << "\"";
  }
}

string MapObject::Dump() const {
  string map_string;

  if (map_.empty()) {
    map_string = "{}";
  } else {
    map_string = "{";
    for (unordered_map<string, PeerObject*>::const_iterator it = map_.begin();
         it != map_.end(); ++it) {
      if (it != map_.begin()) {
        map_string += ",";
      }

      StringAppendF(&map_string, " \"%s\": %s", CEscape(it->first).c_str(),
                    it->second->Dump().c_str());
    }
    map_string += " }";
  }

  return StringPrintf("{ \"type\": \"MapObject\", \"map\": %s }",
                      map_string.c_str());
}

// static
MapObject* MapObject::ParseMapProto(const MapProto& map_proto,
                                    DeserializationContext* context) {
  CHECK(context != NULL);

  MapObject* const new_object = new MapObject();
  unordered_map<string, PeerObject*>* const the_map = &new_object->map_;

  for (int i = 0; i < map_proto.entry_size(); ++i) {
    const MapEntryProto& entry_proto = map_proto.entry(i);

    const string& key = entry_proto.key();
    const int64 object_index = entry_proto.value_object_index();
    PeerObject* const peer_object = context->GetPeerObjectByIndex(object_index);

    CHECK(the_map->insert(make_pair(key, peer_object)).second);
  }

  return new_object;
}

void MapObject::PopulateObjectProto(ObjectProto* object_proto,
                                    SerializationContext* context) const {
  CHECK(context != NULL);

  MapProto* const map_proto = object_proto->mutable_map_object();

  for (unordered_map<string, PeerObject*>::const_iterator it = map_.begin();
       it != map_.end(); ++it) {
    MapEntryProto* const entry_proto = map_proto->add_entry();

    PeerObject* const peer_object = it->second;
    const int object_index = context->GetIndexForPeerObject(peer_object);

    entry_proto->set_key(it->first);
    entry_proto->set_value_object_index(object_index);
  }
}

RangeIteratorObject::RangeIteratorObject(int64 limit, int64 start)
    : limit_(limit),
      i_(start) {
  CHECK_LE(start, limit);
}

LocalObject* RangeIteratorObject::Clone() const {
  int64 i_temp = 0;
  {
    MutexLock lock(&i_mu_);
    i_temp = i_;
  }

  return new RangeIteratorObject(limit_, i_temp);
}

void RangeIteratorObject::InvokeMethod(Thread* thread,
                                       PeerObject* peer_object,
                                       const string& method_name,
                                       const vector<Value>& parameters,
                                       Value* return_value) {
  CHECK(return_value != NULL);

  if (method_name == "has_next") {
    CHECK_EQ(parameters.size(), 0u);

    MutexLock lock(&i_mu_);
    CHECK_LE(i_, limit_);
    return_value->set_bool_value(0, i_ < limit_);
  } else if (method_name == "get_next") {
    CHECK_EQ(parameters.size(), 0u);

    MutexLock lock(&i_mu_);
    CHECK_LT(i_, limit_);
    return_value->set_int64_value(0, i_);
    ++i_;
  } else {
    LOG(FATAL) << "Unsupported method: \"" << CEscape(method_name) << "\"";
  }
}

string RangeIteratorObject::Dump() const {
  int64 i_temp = 0;
  {
    MutexLock lock(&i_mu_);
    i_temp = i_;
  }

  return StringPrintf(
      "{ \"type\": \"RangeIteratorObject\", \"limit\": %" PRId64 ", "
      "\"i\": %" PRId64 " }",
      limit_, i_temp);
}

// static
RangeIteratorObject* RangeIteratorObject::ParseRangeIteratorProto(
    const RangeIteratorProto& range_iterator_proto) {
  return new RangeIteratorObject(range_iterator_proto.limit(),
                                 range_iterator_proto.i());
}

void RangeIteratorObject::PopulateObjectProto(
    ObjectProto* object_proto, SerializationContext* context) const {
  RangeIteratorProto* const range_iterator_proto =
      object_proto->mutable_range_iterator_object();

  range_iterator_proto->set_limit(limit_);

  MutexLock lock(&i_mu_);
  range_iterator_proto->set_i(i_);
}

void Function::InvokeMethod(Thread* thread,
                            PeerObject* peer_object,
                            const string& method_name,
                            const vector<Value>& parameters,
                            Value* return_value) {
  CHECK(thread != NULL);
  CHECK(return_value != NULL);

  if (method_name == "call") {
    CHECK_EQ(parameters.size(), 2u);

    PeerObject* const symbol_table_object = parameters[0].peer_object();
    PeerObject* const parameter_list_object = parameters[1].peer_object();

    Value length_value;
    if (!thread->CallMethod(parameter_list_object, "length", vector<Value>(),
                            &length_value)) {
      return;
    }

    const int64 parameter_count = length_value.int64_value();
    vector<PeerObject*> param_objects(parameter_count);

    for (int64 i = 0; i < parameter_count; ++i) {
      vector<Value> get_at_params(1);
      get_at_params[0].set_int64_value(0, i);

      Value list_item_value;
      if (!thread->CallMethod(parameter_list_object, "get_at", get_at_params,
                              &list_item_value)) {
        return;
      }

      param_objects[i] = list_item_value.peer_object();
    }

    PeerObject* const return_object = Call(symbol_table_object, thread,
                                           param_objects);
    if (return_object == NULL) {
      return;
    }
    return_value->set_peer_object(0, return_object);
  } else {
    LOG(FATAL) << "Unsupported method: \"" << CEscape(method_name) << "\"";
  }
}

ListFunction::ListFunction() {
}

LocalObject* ListFunction::Clone() const {
  return new ListFunction();
}

string ListFunction::Dump() const {
  return "{ \"type\": \"ListFunction\" }";
}

void ListFunction::PopulateObjectProto(ObjectProto* object_proto,
                                       SerializationContext* context) const {
  object_proto->mutable_list_function();
}

PeerObject* ListFunction::Call(PeerObject* symbol_table_object, Thread* thread,
                               const vector<PeerObject*>& parameters) const {
  CHECK(thread != NULL);

  LocalObjectImpl* const local_object = new ListObject(parameters);
  return thread->CreatePeerObject(local_object);
}

SetVariableFunction::SetVariableFunction() {
}

LocalObject* SetVariableFunction::Clone() const {
  return new SetVariableFunction();
}

string SetVariableFunction::Dump() const {
  return "{ \"type\": \"SetVariableFunction\" }";
}

void SetVariableFunction::PopulateObjectProto(
    ObjectProto* object_proto, SerializationContext* context) const {
  object_proto->mutable_set_variable_function();
}

PeerObject* SetVariableFunction::Call(
    PeerObject* symbol_table_object, Thread* thread,
    const vector<PeerObject*>& parameters) const {
  CHECK(thread != NULL);
  CHECK_EQ(parameters.size(), 2u);

  Value variable_name;
  if (!thread->CallMethod(parameters[0], "get_string", vector<Value>(),
                          &variable_name)) {
    return NULL;
  }

  PeerObject* const object = parameters[1];

  if (!SetVariable(symbol_table_object, thread, variable_name.string_value(),
                   object)) {
    return NULL;
  }

  return object;
}

ForFunction::ForFunction() {
}

LocalObject* ForFunction::Clone() const {
  return new ForFunction();
}

string ForFunction::Dump() const {
  return "{ \"type\": \"ForFunction\" }";
}

void ForFunction::PopulateObjectProto(ObjectProto* object_proto,
                                      SerializationContext* context) const {
  object_proto->mutable_for_function();
}

PeerObject* ForFunction::Call(PeerObject* symbol_table_object, Thread* thread,
                              const vector<PeerObject*>& parameters) const {
  CHECK(thread != NULL);
  CHECK_EQ(parameters.size(), 3u);

  Value variable_name;
  if (!thread->CallMethod(parameters[0], "get_string", vector<Value>(),
                          &variable_name)) {
    return NULL;
  }

  PeerObject* const iter = parameters[1];
  PeerObject* const expression = parameters[2];

  vector<Value> eval_parameters(1);
  eval_parameters[0].set_peer_object(0, symbol_table_object);

  for (;;) {
    Value has_next;
    if (!thread->CallMethod(iter, "has_next", vector<Value>(), &has_next)) {
      return NULL;
    }

    if (!has_next.bool_value()) {
      break;
    }

    Value iter_value;
    if (!thread->CallMethod(iter, "get_next", vector<Value>(), &iter_value)) {
      return NULL;
    }

    if (!EnterScope(symbol_table_object, thread)) {
      return NULL;
    }

    if (!SetVariable(
            symbol_table_object, thread, variable_name.string_value(),
            thread->CreatePeerObject(
                new IntObject(iter_value.int64_value())))) {
      return NULL;
    }

    Value dummy;
    if (!thread->CallMethod(expression, "eval", eval_parameters, &dummy)) {
      return NULL;
    }

    if (!LeaveScope(symbol_table_object, thread)) {
      return NULL;
    }
  }

  return thread->CreatePeerObject(new NoneObject());
}

RangeFunction::RangeFunction() {
}

LocalObject* RangeFunction::Clone() const {
  return new RangeFunction();
}

string RangeFunction::Dump() const {
  return "{ \"type\": \"RangeFunction\" }";
}

void RangeFunction::PopulateObjectProto(ObjectProto* object_proto,
                                        SerializationContext* context) const {
  object_proto->mutable_range_function();
}

PeerObject* RangeFunction::Call(PeerObject* symbol_table_object, Thread* thread,
                                const vector<PeerObject*>& parameters) const {
  CHECK(thread != NULL);
  CHECK_EQ(parameters.size(), 1u);

  Value limit;
  if (!thread->CallMethod(parameters[0], "get_int", vector<Value>(), &limit)) {
    return NULL;
  }

  return thread->CreatePeerObject(new RangeIteratorObject(limit.int64_value(),
                                                          0));
}

PrintFunction::PrintFunction() {
}

LocalObject* PrintFunction::Clone() const {
  return new PrintFunction();
}

string PrintFunction::Dump() const {
  return "{ \"type\": \"PrintFunction\" }";
}

void PrintFunction::PopulateObjectProto(ObjectProto* object_proto,
                                        SerializationContext* context) const {
  object_proto->mutable_print_function();
}

PeerObject* PrintFunction::Call(PeerObject* symbol_table_object, Thread* thread,
                                const vector<PeerObject*>& parameters) const {
  CHECK(thread != NULL);

  for (vector<PeerObject*>::const_iterator it = parameters.begin();
       it != parameters.end(); ++it) {
    if (it != parameters.begin()) {
      printf(" ");
    }

    Value str;
    if (!thread->CallMethod(*it, "get_string", vector<Value>(), &str)) {
      return NULL;
    }

    printf("%s", str.string_value().c_str());
  }

  printf("\n");

  return thread->CreatePeerObject(new NoneObject());
}

AddFunction::AddFunction() {
}

LocalObject* AddFunction::Clone() const {
  return new AddFunction();
}

string AddFunction::Dump() const {
  return "{ \"type\": \"AddFunction\" }";
}

void AddFunction::PopulateObjectProto(ObjectProto* object_proto,
                                      SerializationContext* context) const {
  object_proto->mutable_add_function();
}

PeerObject* AddFunction::Call(PeerObject* symbol_table_object, Thread* thread,
                              const vector<PeerObject*>& parameters) const {
  CHECK(thread != NULL);

  int64 sum = 0;

  for (vector<PeerObject*>::const_iterator it = parameters.begin();
       it != parameters.end(); ++it) {
    Value number;
    if (!thread->CallMethod(*it, "get_int", vector<Value>(), &number)) {
      return NULL;
    }

    sum += number.int64_value();
  }

  return thread->CreatePeerObject(new IntObject(sum));
}

BeginTranFunction::BeginTranFunction() {
}

LocalObject* BeginTranFunction::Clone() const {
  return new BeginTranFunction();
}

string BeginTranFunction::Dump() const {
  return "{ \"type\": \"BeginTranFunction\" }";
}

void BeginTranFunction::PopulateObjectProto(
    ObjectProto* object_proto, SerializationContext* context) const {
  object_proto->mutable_begin_tran_function();
}

PeerObject* BeginTranFunction::Call(
    PeerObject* symbol_table_object, Thread* thread,
    const vector<PeerObject*>& parameters) const {
  CHECK(thread != NULL);
  CHECK_EQ(parameters.size(), 0u);

  if (!thread->BeginTransaction()) {
    return NULL;
  }

  return thread->CreatePeerObject(new NoneObject());
}

EndTranFunction::EndTranFunction() {
}

LocalObject* EndTranFunction::Clone() const {
  return new EndTranFunction();
}

string EndTranFunction::Dump() const {
  return "{ \"type\": \"EndTranFunction\" }";
}

void EndTranFunction::PopulateObjectProto(ObjectProto* object_proto,
                                          SerializationContext* context) const {
  object_proto->mutable_end_tran_function();
}

PeerObject* EndTranFunction::Call(PeerObject* symbol_table_object,
                                  Thread* thread,
                                  const vector<PeerObject*>& parameters) const {
  CHECK(thread != NULL);
  CHECK_EQ(parameters.size(), 0u);

  if (!thread->EndTransaction()) {
    return NULL;
  }

  return thread->CreatePeerObject(new NoneObject());
}

IfFunction::IfFunction() {
}

LocalObject* IfFunction::Clone() const {
  return new IfFunction();
}

string IfFunction::Dump() const {
  return "{ \"type\": \"IfFunction\" }";
}

void IfFunction::PopulateObjectProto(ObjectProto* object_proto,
                                     SerializationContext* context) const {
  object_proto->mutable_if_function();
}

PeerObject* IfFunction::Call(PeerObject* symbol_table_object, Thread* thread,
                             const vector<PeerObject*>& parameters) const {
  CHECK(thread != NULL);
  CHECK_GE(parameters.size(), 2u);
  CHECK_LE(parameters.size(), 3u);

  Value condition;
  if (!thread->CallMethod(parameters[0], "get_bool", vector<Value>(),
                          &condition)) {
    return NULL;
  }

  PeerObject* expression = NULL;

  if (condition.bool_value()) {
    expression = parameters[1];
  } else {
    if (parameters.size() < 3u) {
      return thread->CreatePeerObject(new NoneObject());
    }

    expression = parameters[2];
  }

  vector<Value> eval_parameters(1);
  eval_parameters[0].set_peer_object(0, symbol_table_object);

  Value result;
  if (!thread->CallMethod(expression, "eval", eval_parameters, &result)) {
    return NULL;
  }

  return result.peer_object();
}

NotFunction::NotFunction() {
}

LocalObject* NotFunction::Clone() const {
  return new NotFunction();
}

string NotFunction::Dump() const {
  return "{ \"type\": \"NotFunction\" }";
}

void NotFunction::PopulateObjectProto(ObjectProto* object_proto,
                                      SerializationContext* context) const {
  object_proto->mutable_not_function();
}

PeerObject* NotFunction::Call(PeerObject* symbol_table_object, Thread* thread,
                              const vector<PeerObject*>& parameters) const {
  CHECK(thread != NULL);
  CHECK_EQ(parameters.size(), 1u);

  Value condition;
  if (!thread->CallMethod(parameters[0], "get_bool", vector<Value>(),
                          &condition)) {
    return NULL;
  }

  return thread->CreatePeerObject(new BoolObject(!condition.bool_value()));
}

IsSetFunction::IsSetFunction() {
}

LocalObject* IsSetFunction::Clone() const {
  return new IsSetFunction();
}

string IsSetFunction::Dump() const {
  return "{ \"type\": \"IsSetFunction\" }";
}

void IsSetFunction::PopulateObjectProto(ObjectProto* object_proto,
                                        SerializationContext* context) const {
  object_proto->mutable_is_set_function();
}

PeerObject* IsSetFunction::Call(PeerObject* symbol_table_object, Thread* thread,
                                const vector<PeerObject*>& parameters) const {
  CHECK(thread != NULL);
  CHECK_EQ(parameters.size(), 1u);

  Value variable_name;
  if (!thread->CallMethod(parameters[0], "get_string", vector<Value>(),
                          &variable_name)) {
    return NULL;
  }

  bool is_set = false;
  if (!IsVariableSet(symbol_table_object, thread, variable_name.string_value(),
                     &is_set)) {
    return NULL;
  }

  return thread->CreatePeerObject(new BoolObject(is_set));
}

WhileFunction::WhileFunction() {
}

LocalObject* WhileFunction::Clone() const {
  return new WhileFunction();
}

string WhileFunction::Dump() const {
  return "{ \"type\": \"WhileFunction\" }";
}

void WhileFunction::PopulateObjectProto(ObjectProto* object_proto,
                                        SerializationContext* context) const {
  object_proto->mutable_while_function();
}

PeerObject* WhileFunction::Call(PeerObject* symbol_table_object, Thread* thread,
                                const vector<PeerObject*>& parameters) const {
  CHECK(thread != NULL);
  CHECK_EQ(parameters.size(), 2u);

  PeerObject* const condition_expression = parameters[0];
  PeerObject* const expression = parameters[1];

  vector<Value> eval_parameters(1);
  eval_parameters[0].set_peer_object(0, symbol_table_object);

  for (;;) {
    Value condition_object;
    if (!thread->CallMethod(condition_expression, "eval", eval_parameters,
                            &condition_object)) {
      return NULL;
    }

    Value condition;
    if (!thread->CallMethod(condition_object.peer_object(), "get_bool",
                            vector<Value>(), &condition)) {
      return NULL;
    }

    if (!condition.bool_value()) {
      break;
    }

    if (!EnterScope(symbol_table_object, thread)) {
      return NULL;
    }

    Value dummy;
    if (!thread->CallMethod(expression, "eval", eval_parameters, &dummy)) {
      return NULL;
    }

    if (!LeaveScope(symbol_table_object, thread)) {
      return NULL;
    }
  }

  return thread->CreatePeerObject(new NoneObject());
}

LessThanFunction::LessThanFunction() {
}

LocalObject* LessThanFunction::Clone() const {
  return new LessThanFunction();
}

string LessThanFunction::Dump() const {
  return "{ \"type\": \"LessThanFunction\" }";
}

void LessThanFunction::PopulateObjectProto(
    ObjectProto* object_proto, SerializationContext* context) const {
  object_proto->mutable_less_than_function();
}

PeerObject* LessThanFunction::Call(
    PeerObject* symbol_table_object, Thread* thread,
    const vector<PeerObject*>& parameters) const {
  CHECK(thread != NULL);
  CHECK_EQ(parameters.size(), 2u);

  int64 operands[2];

  for (int i = 0; i < 2; ++i) {
    Value number;
    if (!thread->CallMethod(parameters[i], "get_int", vector<Value>(),
                            &number)) {
      return NULL;
    }

    operands[i] = number.int64_value();
  }

  return thread->CreatePeerObject(new BoolObject(operands[0] < operands[1]));
}

LenFunction::LenFunction() {
}

LocalObject* LenFunction::Clone() const {
  return new LenFunction();
}

string LenFunction::Dump() const {
  return "{ \"type\": \"LenFunction\" }";
}

void LenFunction::PopulateObjectProto(ObjectProto* object_proto,
                                      SerializationContext* context) const {
  object_proto->mutable_len_function();
}

PeerObject* LenFunction::Call(PeerObject* symbol_table_object, Thread* thread,
                              const vector<PeerObject*>& parameters) const {
  CHECK(thread != NULL);
  CHECK_EQ(parameters.size(), 1u);

  Value length;
  if (!thread->CallMethod(parameters[0], "length", vector<Value>(), &length)) {
    return NULL;
  }

  return thread->CreatePeerObject(new IntObject(length.int64_value()));
}

AppendFunction::AppendFunction() {
}

LocalObject* AppendFunction::Clone() const {
  return new AppendFunction();
}

string AppendFunction::Dump() const {
  return "{ \"type\": \"AppendFunction\" }";
}

void AppendFunction::PopulateObjectProto(ObjectProto* object_proto,
                                         SerializationContext* context) const {
  object_proto->mutable_append_function();
}

PeerObject* AppendFunction::Call(PeerObject* symbol_table_object,
                                 Thread* thread,
                                 const vector<PeerObject*>& parameters) const {
  CHECK(thread != NULL);
  CHECK_EQ(parameters.size(), 2u);

  PeerObject* const the_list = parameters[0];
  PeerObject* const object = parameters[1];

  vector<Value> append_params(1);
  append_params[0].set_peer_object(0, object);

  Value dummy;
  if (!thread->CallMethod(the_list, "append", append_params, &dummy)) {
    return NULL;
  }

  return thread->CreatePeerObject(new NoneObject());
}

GetAtFunction::GetAtFunction() {
}

LocalObject* GetAtFunction::Clone() const {
  return new GetAtFunction();
}

string GetAtFunction::Dump() const {
  return "{ \"type\": \"GetAtFunction\" }";
}

void GetAtFunction::PopulateObjectProto(ObjectProto* object_proto,
                                        SerializationContext* context) const {
  object_proto->mutable_get_at_function();
}

PeerObject* GetAtFunction::Call(PeerObject* symbol_table_object, Thread* thread,
                                const vector<PeerObject*>& parameters) const {
  CHECK(thread != NULL);
  CHECK_EQ(parameters.size(), 2u);

  Value index;
  if (!thread->CallMethod(parameters[1], "get_int", vector<Value>(), &index)) {
    return NULL;
  }

  vector<Value> get_at_params(1);
  get_at_params[0] = index;

  Value item;
  if (!thread->CallMethod(parameters[0], "get_at", get_at_params, &item)) {
    return NULL;
  }

  return item.peer_object();
}

MapIsSetFunction::MapIsSetFunction() {
}

LocalObject* MapIsSetFunction::Clone() const {
  return new MapIsSetFunction();
}

string MapIsSetFunction::Dump() const {
  return "{ \"type\": \"MapIsSetFunction\" }";
}

void MapIsSetFunction::PopulateObjectProto(
    ObjectProto* object_proto, SerializationContext* context) const {
  object_proto->mutable_map_is_set_function();
}

PeerObject* MapIsSetFunction::Call(
    PeerObject* symbol_table_object, Thread* thread,
    const vector<PeerObject*>& parameters) const {
  CHECK(thread != NULL);
  CHECK_EQ(parameters.size(), 2u);

  Value key;
  if (!thread->CallMethod(parameters[1], "get_string", vector<Value>(), &key)) {
    return NULL;
  }

  vector<Value> is_set_params(1);
  is_set_params[0] = key;

  Value result;
  if (!thread->CallMethod(parameters[0], "is_set", is_set_params, &result)) {
    return NULL;
  }

  return thread->CreatePeerObject(new BoolObject(result.bool_value()));
}

MapGetFunction::MapGetFunction() {
}

LocalObject* MapGetFunction::Clone() const {
  return new MapGetFunction();
}

string MapGetFunction::Dump() const {
  return "{ \"type\": \"MapGetFunction\" }";
}

void MapGetFunction::PopulateObjectProto(ObjectProto* object_proto,
                                         SerializationContext* context) const {
  object_proto->mutable_map_get_function();
}

PeerObject* MapGetFunction::Call(PeerObject* symbol_table_object,
                                 Thread* thread,
                                 const vector<PeerObject*>& parameters) const {
  CHECK(thread != NULL);
  CHECK_EQ(parameters.size(), 2u);

  Value key;
  if (!thread->CallMethod(parameters[1], "get_string", vector<Value>(), &key)) {
    return NULL;
  }

  vector<Value> get_params(1);
  get_params[0] = key;

  Value result;
  if (!thread->CallMethod(parameters[0], "get", get_params, &result)) {
    return NULL;
  }

  return result.peer_object();
}

MapSetFunction::MapSetFunction() {
}

LocalObject* MapSetFunction::Clone() const {
  return new MapSetFunction();
}

string MapSetFunction::Dump() const {
  return "{ \"type\": \"MapSetFunction\" }";
}

void MapSetFunction::PopulateObjectProto(ObjectProto* object_proto,
                                         SerializationContext* context) const {
  object_proto->mutable_map_set_function();
}

PeerObject* MapSetFunction::Call(PeerObject* symbol_table_object,
                                 Thread* thread,
                                 const vector<PeerObject*>& parameters) const {
  CHECK(thread != NULL);
  CHECK_EQ(parameters.size(), 3u);

  Value key;
  if (!thread->CallMethod(parameters[1], "get_string", vector<Value>(), &key)) {
    return NULL;
  }

  vector<Value> set_params(2);
  set_params[0] = key;
  set_params[1].set_peer_object(0, parameters[2]);

  Value result;
  if (!thread->CallMethod(parameters[0], "set", set_params, &result)) {
    return NULL;
  }

  return thread->CreatePeerObject(new NoneObject());
}

}  // namespace toy_lang
}  // namespace floating_temple