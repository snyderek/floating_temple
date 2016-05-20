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
#include "toy_lang/zoo/add_function.h"
#include "toy_lang/zoo/begin_tran_function.h"
#include "toy_lang/zoo/bool_object.h"
#include "toy_lang/zoo/end_tran_function.h"
#include "toy_lang/zoo/for_function.h"
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
#include "toy_lang/zoo/while_function.h"
#include "util/dump_context.h"

using std::string;
using std::unordered_map;
using std::vector;

namespace floating_temple {

class ObjectReference;

namespace toy_lang {
namespace {

bool AddSymbol(Thread* thread, const string& name,
               LocalObjectImpl* local_object) {
  // TODO(dss): Implement this.
  return false;
}

#define ADD_SYMBOL(name, local_object) \
  do { \
    if (!AddSymbol(thread, name, local_object)) { \
      return false; \
    } \
  } while (false)

bool PopulateSymbolTable(Thread* thread,
                         ObjectReference* shared_map_object) {
  CHECK(thread != nullptr);

  if (!thread->BeginTransaction()) {
    return false;
  }

  // TODO(dss): Add the "shared" map to the symbol table.

  ADD_SYMBOL("false", new BoolObject(false));
  ADD_SYMBOL("true", new BoolObject(true));
  ADD_SYMBOL("list", new ListFunction());
  ADD_SYMBOL("set", new SetVariableFunction());
  ADD_SYMBOL("for", new ForFunction());
  ADD_SYMBOL("range", new RangeFunction());
  ADD_SYMBOL("print", new PrintFunction());
  ADD_SYMBOL("add", new AddFunction());
  ADD_SYMBOL("begin_tran", new BeginTranFunction());
  ADD_SYMBOL("end_tran", new EndTranFunction());
  ADD_SYMBOL("if", new IfFunction());
  ADD_SYMBOL("not", new NotFunction());
  ADD_SYMBOL("while", new WhileFunction());
  ADD_SYMBOL("lt", new LessThanFunction());
  ADD_SYMBOL("len", new LenFunction());
  ADD_SYMBOL("list.append", new ListAppendFunction());
  ADD_SYMBOL("list.get", new ListGetFunction());
  ADD_SYMBOL("map.is_set", new MapIsSetFunction());
  ADD_SYMBOL("map.get", new MapGetFunction());
  ADD_SYMBOL("map.set", new MapSetFunction());

  if (!thread->EndTransaction()) {
    return false;
  }

  return true;
}

#undef ADD_SYMBOL

}  // namespace

ProgramObject::ProgramObject(Expression* expression)
    : expression_(CHECK_NOTNULL(expression)) {
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

  ObjectReference* const shared_map_object = thread->CreateVersionedObject(
      new MapObject(), "shared");

  if (!PopulateSymbolTable(thread, shared_map_object)) {
    return;
  }

  // TODO(dss): Populate this map.
  const unordered_map<int, ObjectReference*> symbol_bindings;
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

}  // namespace toy_lang
}  // namespace floating_temple
