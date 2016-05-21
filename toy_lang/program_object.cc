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

#include "toy_lang/program_object.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/logging.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "toy_lang/expression.h"
#include "toy_lang/hidden_symbols.h"
#include "toy_lang/proto/serialization.pb.h"
#include "toy_lang/symbol_table.h"
#include "toy_lang/zoo/add_function.h"
#include "toy_lang/zoo/begin_tran_function.h"
#include "toy_lang/zoo/bool_object.h"
#include "toy_lang/zoo/end_tran_function.h"
#include "toy_lang/zoo/for_function.h"
#include "toy_lang/zoo/get_variable_function.h"
#include "toy_lang/zoo/if_function.h"
#include "toy_lang/zoo/len_function.h"
#include "toy_lang/zoo/less_than_function.h"
#include "toy_lang/zoo/list_append_function.h"
#include "toy_lang/zoo/list_function.h"
#include "toy_lang/zoo/list_get_function.h"
#include "toy_lang/zoo/map_get_function.h"
#include "toy_lang/zoo/map_is_set_function.h"
#include "toy_lang/zoo/map_object.h"
#include "toy_lang/zoo/map_set_function.h"
#include "toy_lang/zoo/not_function.h"
#include "toy_lang/zoo/print_function.h"
#include "toy_lang/zoo/range_function.h"
#include "toy_lang/zoo/set_variable_function.h"
#include "toy_lang/zoo/variable_object.h"
#include "toy_lang/zoo/while_function.h"
#include "util/dump_context.h"

using std::string;
using std::unordered_map;
using std::vector;

namespace floating_temple {

class ObjectReference;

namespace toy_lang {
namespace {

void AddSymbol(SymbolTable* symbol_table,
               Thread* thread,
               const string& name,
               LocalObjectImpl* local_object,
               unordered_map<int, ObjectReference*>* symbol_bindings) {
  CHECK(!name.empty());

  ObjectReference* const built_in_object = thread->CreateVersionedObject(
      local_object, name);

  const int symbol_id = symbol_table->AddExternalSymbol(name);
  ObjectReference* const variable_object = thread->CreateVersionedObject(
      new VariableObject(built_in_object), "");
  CHECK(symbol_bindings->emplace(symbol_id, variable_object).second);
}

}  // namespace

ProgramObject::ProgramObject(SymbolTable* symbol_table, Expression* expression,
                             const HiddenSymbols& hidden_symbols)
    : symbol_table_(CHECK_NOTNULL(symbol_table)),
      expression_(CHECK_NOTNULL(expression)),
      hidden_symbols_(hidden_symbols) {
}

ProgramObject::~ProgramObject() {
}

void ProgramObject::InvokeMethod(Thread* thread,
                                 ObjectReference* object_reference,
                                 const string& method_name,
                                 const vector<Value>& parameters,
                                 Value* return_value) {
  CHECK(thread != nullptr);
  CHECK_EQ(method_name, "run");
  CHECK(return_value != nullptr);

  unordered_map<int, ObjectReference*> symbol_bindings;
  if (!CreateBuiltInObjects(symbol_table_, thread, &symbol_bindings)) {
    return;
  }

  ObjectReference* const code_block_object = expression_->Evaluate(
      symbol_bindings, thread);

  const vector<Value> eval_parameters;
  Value dummy;
  if (!thread->CallMethod(code_block_object, "eval", eval_parameters, &dummy)) {
    return;
  }

  return_value->set_empty(0);
}

void ProgramObject::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("ProgramObject");

  dc->AddString("expression");
  dc->AddString(expression_->DebugString());

  dc->End();
}

bool ProgramObject::CreateBuiltInObjects(
    SymbolTable* symbol_table, Thread* thread,
    unordered_map<int, ObjectReference*>* symbol_bindings) {
  CHECK(symbol_table != nullptr);
  CHECK(thread != nullptr);
  CHECK(symbol_bindings != nullptr);

  if (!thread->BeginTransaction()) {
    return false;
  }

  ObjectReference* const get_variable_function = thread->CreateVersionedObject(
      new GetVariableFunction(), "get");
  ObjectReference* const set_variable_function = thread->CreateVersionedObject(
      new SetVariableFunction(), "set");

  CHECK(symbol_bindings->emplace(hidden_symbols_.get_variable_symbol_id,
                                 get_variable_function).second);
  CHECK(symbol_bindings->emplace(hidden_symbols_.set_variable_symbol_id,
                                 set_variable_function).second);

  // TODO(dss): Make these unversioned objects. There's no reason to record
  // method calls on any of these objects, because they're constant. (The
  // "shared" map object should still be versioned, however.)
  AddSymbol(symbol_table, thread, "false", new BoolObject(false),
            symbol_bindings);
  AddSymbol(symbol_table, thread, "true", new BoolObject(true),
            symbol_bindings);
  AddSymbol(symbol_table, thread, "list", new ListFunction(),
            symbol_bindings);
  AddSymbol(symbol_table, thread, "for", new ForFunction(),
            symbol_bindings);
  AddSymbol(symbol_table, thread, "range", new RangeFunction(),
            symbol_bindings);
  AddSymbol(symbol_table, thread, "print", new PrintFunction(),
            symbol_bindings);
  AddSymbol(symbol_table, thread, "add", new AddFunction(),
            symbol_bindings);
  AddSymbol(symbol_table, thread, "begin_tran", new BeginTranFunction(),
            symbol_bindings);
  AddSymbol(symbol_table, thread, "end_tran", new EndTranFunction(),
            symbol_bindings);
  AddSymbol(symbol_table, thread, "if", new IfFunction(),
            symbol_bindings);
  AddSymbol(symbol_table, thread, "not", new NotFunction(),
            symbol_bindings);
  AddSymbol(symbol_table, thread, "while", new WhileFunction(),
            symbol_bindings);
  AddSymbol(symbol_table, thread, "lt", new LessThanFunction(),
            symbol_bindings);
  AddSymbol(symbol_table, thread, "len", new LenFunction(),
            symbol_bindings);
  AddSymbol(symbol_table, thread, "list.append", new ListAppendFunction(),
            symbol_bindings);
  AddSymbol(symbol_table, thread, "list.get", new ListGetFunction(),
            symbol_bindings);
  AddSymbol(symbol_table, thread, "map.is_set", new MapIsSetFunction(),
            symbol_bindings);
  AddSymbol(symbol_table, thread, "map.get", new MapGetFunction(),
            symbol_bindings);
  AddSymbol(symbol_table, thread, "map.set", new MapSetFunction(),
            symbol_bindings);

  AddSymbol(symbol_table, thread, "shared", new MapObject(), symbol_bindings);

  if (!thread->EndTransaction()) {
    return false;
  }

  return true;
}

}  // namespace toy_lang
}  // namespace floating_temple
