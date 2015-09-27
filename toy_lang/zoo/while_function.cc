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

#include "toy_lang/zoo/while_function.h"

#include <vector>

#include "base/logging.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "toy_lang/proto/serialization.pb.h"
#include "toy_lang/symbol_table.h"
#include "toy_lang/zoo/none_object.h"
#include "util/dump_context.h"

using std::vector;

namespace floating_temple {
namespace toy_lang {

WhileFunction::WhileFunction() {
}

VersionedLocalObject* WhileFunction::Clone() const {
  return new WhileFunction();
}

void WhileFunction::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();
  dc->AddString("type");
  dc->AddString("WhileFunction");
  dc->End();
}

void WhileFunction::PopulateObjectProto(ObjectProto* object_proto,
                                        SerializationContext* context) const {
  object_proto->mutable_while_function();
}

ObjectReference* WhileFunction::Call(
    ObjectReference* symbol_table_object, Thread* thread,
    const vector<ObjectReference*>& parameters) const {
  CHECK(thread != nullptr);
  CHECK_EQ(parameters.size(), 2u);

  ObjectReference* const condition_expression = parameters[0];
  ObjectReference* const expression = parameters[1];

  vector<Value> eval_parameters(1);
  eval_parameters[0].set_object_reference(0, symbol_table_object);

  for (;;) {
    Value condition_object;
    if (!thread->CallMethod(condition_expression, "eval", eval_parameters,
                            &condition_object)) {
      return nullptr;
    }

    Value condition;
    if (!thread->CallMethod(condition_object.object_reference(), "get_bool",
                            vector<Value>(), &condition)) {
      return nullptr;
    }

    if (!condition.bool_value()) {
      break;
    }

    if (!EnterScope(symbol_table_object, thread)) {
      return nullptr;
    }

    Value dummy;
    if (!thread->CallMethod(expression, "eval", eval_parameters, &dummy)) {
      return nullptr;
    }

    if (!LeaveScope(symbol_table_object, thread)) {
      return nullptr;
    }
  }

  return thread->CreateVersionedObject(new NoneObject(), "");
}

}  // namespace toy_lang
}  // namespace floating_temple