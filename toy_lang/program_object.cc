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
#include <vector>

#include "base/logging.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "toy_lang/expression.h"
#include "toy_lang/local_object_impl.h"
#include "toy_lang/proto/serialization.pb.h"
#include "toy_lang/symbol_table.h"

using std::shared_ptr;
using std::string;
using std::vector;

namespace floating_temple {

class ObjectReference;

namespace toy_lang {
namespace {

bool AddSymbol(ObjectReference* symbol_table_object,
               Thread* thread,
               const string& name,
               LocalObjectImpl* local_object) {
  return SetVariable(symbol_table_object, thread, name,
                     thread->CreateVersionedObject(local_object, ""));
}

#define ADD_SYMBOL(name, local_object) \
  do { \
    if (!AddSymbol(symbol_table_object, thread, name, local_object)) { \
      return false; \
    } \
  } while (false)

bool PopulateSymbolTable(ObjectReference* symbol_table_object, Thread* thread,
                         ObjectReference* shared_map_object) {
  CHECK(thread != nullptr);

  if (!thread->BeginTransaction()) {
    return false;
  }

  if (!SetVariable(symbol_table_object, thread, "shared", shared_map_object)) {
    return false;
  }

  ADD_SYMBOL("false", new BoolObject(false));
  ADD_SYMBOL("true", new BoolObject(true));
  ADD_SYMBOL("list", new ListFunction());
  ADD_SYMBOL("set", new SetVariableFunction());
  ADD_SYMBOL("for", new ForFunction());
  ADD_SYMBOL("range", new RangeFunction());
  ADD_SYMBOL("print", new PrintFunction());
  ADD_SYMBOL("+", new AddFunction());
  ADD_SYMBOL("begin_tran", new BeginTranFunction());
  ADD_SYMBOL("end_tran", new EndTranFunction());
  ADD_SYMBOL("if", new IfFunction());
  ADD_SYMBOL("not", new NotFunction());
  ADD_SYMBOL("is_set", new IsSetFunction());
  ADD_SYMBOL("while", new WhileFunction());
  ADD_SYMBOL("<", new LessThanFunction());
  ADD_SYMBOL("len", new LenFunction());
  ADD_SYMBOL("append", new AppendFunction());
  ADD_SYMBOL("get_at", new GetAtFunction());
  ADD_SYMBOL("map_is_set", new MapIsSetFunction());
  ADD_SYMBOL("map_get", new MapGetFunction());
  ADD_SYMBOL("map_set", new MapSetFunction());

  if (!thread->EndTransaction()) {
    return false;
  }

  return true;
}

#undef ADD_SYMBOL

}  // namespace

ProgramObject::ProgramObject(const shared_ptr<const Expression>& expression)
    : expression_(expression) {
  CHECK(expression.get() != nullptr);
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
  ObjectReference* const expression_object = thread->CreateVersionedObject(
      new ExpressionObject(expression_), "");
  ObjectReference* const symbol_table_object = thread->CreateVersionedObject(
      new SymbolTableObject(), "");

  if (!PopulateSymbolTable(symbol_table_object, thread, shared_map_object)) {
    return;
  }

  vector<Value> eval_parameters(1);
  eval_parameters[0].set_object_reference(0, symbol_table_object);

  Value dummy;
  if (!thread->CallMethod(expression_object, "eval", eval_parameters, &dummy)) {
    return;
  }

  return_value->set_empty(0);
}

string ProgramObject::Dump() const {
  return "{ \"type\": \"ProgramObject\" }";
}

}  // namespace toy_lang
}  // namespace floating_temple
