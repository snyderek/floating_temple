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
#include "toy_lang/symbol_table.h"
#include "toy_lang/zoo/list_object.h"
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

bool ProgramObject::CreateBuiltInObjects(Thread* thread) {
  CHECK(thread != nullptr);

  if (!thread->BeginTransaction()) {
    return false;
  }

  symbol_table_->ResolveExternalSymbols(thread);

  if (!thread->EndTransaction()) {
    return false;
  }

  return true;
}

}  // namespace toy_lang
}  // namespace floating_temple
