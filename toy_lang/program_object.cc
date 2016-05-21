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

ProgramObject::ProgramObject(SymbolTable* symbol_table, Expression* expression)
    : symbol_table_(CHECK_NOTNULL(symbol_table)),
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

  if (!CreateBuiltInObjects(thread)) {
    return;
  }

  unordered_map<int, ObjectReference*> symbol_bindings;
  symbol_table_->GetExternalSymbolBindings(&symbol_bindings);

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

bool ProgramObject::CreateBuiltInObjects(Thread* thread) {
  CHECK(thread != nullptr);

  if (!thread->BeginTransaction()) {
    return false;
  }

  symbol_table_->ResolveExternalSymbol(
      "get", thread->CreateVersionedObject(new GetVariableFunction(), "get"));
  symbol_table_->ResolveExternalSymbol(
      "set", thread->CreateVersionedObject(new SetVariableFunction(), "set"));

  // TODO(dss): Make these unversioned objects. There's no reason to record
  // method calls on any of these objects, because they're constant. (The
  // "shared" map object should still be versioned, however.)
  CreateExternalVariable(thread, "false", new BoolObject(false));
  CreateExternalVariable(thread, "true", new BoolObject(true));
  CreateExternalVariable(thread, "list", new ListFunction());
  CreateExternalVariable(thread, "for", new ForFunction());
  CreateExternalVariable(thread, "range", new RangeFunction());
  CreateExternalVariable(thread, "print", new PrintFunction());
  CreateExternalVariable(thread, "add", new AddFunction());
  CreateExternalVariable(thread, "begin_tran", new BeginTranFunction());
  CreateExternalVariable(thread, "end_tran", new EndTranFunction());
  CreateExternalVariable(thread, "if", new IfFunction());
  CreateExternalVariable(thread, "not", new NotFunction());
  CreateExternalVariable(thread, "while", new WhileFunction());
  CreateExternalVariable(thread, "lt", new LessThanFunction());
  CreateExternalVariable(thread, "len", new LenFunction());
  CreateExternalVariable(thread, "list.append", new ListAppendFunction());
  CreateExternalVariable(thread, "list.get", new ListGetFunction());
  CreateExternalVariable(thread, "map.is_set", new MapIsSetFunction());
  CreateExternalVariable(thread, "map.get", new MapGetFunction());
  CreateExternalVariable(thread, "map.set", new MapSetFunction());

  CreateExternalVariable(thread, "shared", new MapObject());

  if (!thread->EndTransaction()) {
    return false;
  }

  return true;
}

void ProgramObject::CreateExternalVariable(Thread* thread, const string& name,
                                           LocalObjectImpl* local_object) {
  CHECK(thread != nullptr);
  CHECK(!name.empty());

  ObjectReference* const built_in_object = thread->CreateVersionedObject(
      local_object, name);
  ObjectReference* const variable_object = thread->CreateVersionedObject(
      new VariableObject(built_in_object), "");
  symbol_table_->ResolveExternalSymbol(name, variable_object);
}

}  // namespace toy_lang
}  // namespace floating_temple
