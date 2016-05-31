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
#include "toy_lang/zoo/list_object.h"
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
namespace toy_lang {

ProgramObject::ProgramObject(const unordered_map<string, int>& external_symbols,
                             Expression* expression)
    : external_symbols_(external_symbols),
      expression_(CHECK_NOTNULL(expression)) {
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
  symbol_bindings.reserve(external_symbols_.size());

  if (!thread->BeginTransaction()) {
    return;
  }

  ResolveExternalSymbol(thread, "get", false, new GetVariableFunction(),
                        &symbol_bindings);
  ResolveExternalSymbol(thread, "set", false, new SetVariableFunction(),
                        &symbol_bindings);
  ResolveExternalSymbol(thread, "for", false, new ForFunction(),
                        &symbol_bindings);
  ResolveExternalSymbol(thread, "while", false, new WhileFunction(),
                        &symbol_bindings);
  ResolveExternalSymbol(thread, "false", true, new BoolObject(false),
                        &symbol_bindings);
  ResolveExternalSymbol(thread, "true", true, new BoolObject(true),
                        &symbol_bindings);
  ResolveExternalSymbol(thread, "list", true, new ListFunction(),
                        &symbol_bindings);
  ResolveExternalSymbol(thread, "range", true, new RangeFunction(),
                        &symbol_bindings);
  ResolveExternalSymbol(thread, "print", true, new PrintFunction(),
                        &symbol_bindings);
  ResolveExternalSymbol(thread, "add", true, new AddFunction(),
                        &symbol_bindings);
  ResolveExternalSymbol(thread, "begin_tran", true, new BeginTranFunction(),
                        &symbol_bindings);
  ResolveExternalSymbol(thread, "end_tran", true, new EndTranFunction(),
                        &symbol_bindings);
  ResolveExternalSymbol(thread, "if", true, new IfFunction(),
                        &symbol_bindings);
  ResolveExternalSymbol(thread, "not", true, new NotFunction(),
                        &symbol_bindings);
  ResolveExternalSymbol(thread, "lt", true, new LessThanFunction(),
                        &symbol_bindings);
  ResolveExternalSymbol(thread, "len", true, new LenFunction(),
                        &symbol_bindings);
  ResolveExternalSymbol(thread, "list.append", true, new ListAppendFunction(),
                        &symbol_bindings);
  ResolveExternalSymbol(thread, "list.get", true, new ListGetFunction(),
                        &symbol_bindings);
  ResolveExternalSymbol(thread, "map.is_set", true, new MapIsSetFunction(),
                        &symbol_bindings);
  ResolveExternalSymbol(thread, "map.get", true, new MapGetFunction(),
                        &symbol_bindings);
  ResolveExternalSymbol(thread, "map.set", true, new MapSetFunction(),
                        &symbol_bindings);
  ResolveExternalSymbol(thread, "shared", true, new MapObject(),
                        &symbol_bindings);

  if (!thread->EndTransaction()) {
    return;
  }

  ObjectReference* const code_block_object = expression_->Evaluate(
      symbol_bindings, thread);

  ObjectReference* const list_object = thread->CreateVersionedObject(
      new ListObject(vector<ObjectReference*>()), "");
  vector<Value> eval_parameters(1);
  eval_parameters[0].set_object_reference(0, list_object);

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

void ProgramObject::ResolveExternalSymbol(
    Thread* thread,
    const string& symbol_name,
    bool visible,
    VersionedLocalObject* local_object,
    unordered_map<int, ObjectReference*>* symbol_bindings) const {
  CHECK(thread != nullptr);
  CHECK(!symbol_name.empty());
  CHECK(symbol_bindings != nullptr);

  const auto it = external_symbols_.find(symbol_name);
  CHECK(it != external_symbols_.end());
  const int symbol_id = it->second;

  ObjectReference* const built_in_object = thread->CreateVersionedObject(
      local_object, symbol_name);

  ObjectReference* object_reference = nullptr;
  if (visible) {
    object_reference = thread->CreateVersionedObject(
        new VariableObject(built_in_object), "");
  } else {
    object_reference = built_in_object;
  }

  CHECK(symbol_bindings->emplace(symbol_id, object_reference).second);
}

}  // namespace toy_lang
}  // namespace floating_temple
