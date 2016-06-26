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

#include <string>
#include <vector>

#include "base/logging.h"
#include "include/c++/method_context.h"
#include "include/c++/value.h"
#include "toy_lang/proto/serialization.pb.h"
#include "toy_lang/wrap.h"
#include "toy_lang/zoo/list_object.h"
#include "util/dump_context.h"

using std::vector;

namespace floating_temple {

class ObjectReference;

namespace toy_lang {

WhileFunction::WhileFunction() {
}

LocalObject* WhileFunction::Clone() const {
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
    MethodContext* method_context,
    const vector<ObjectReference*>& parameters) const {
  CHECK(method_context != nullptr);
  CHECK_EQ(parameters.size(), 2u);

  ObjectReference* const condition_block = parameters[0];
  ObjectReference* const code_block = parameters[1];

  vector<Value> eval_parameters(1);
  eval_parameters[0].set_object_reference(
      0,
      method_context->CreateObject(new ListObject(vector<ObjectReference*>()),
                                   ""));

  for (;;) {
    Value condition_object;
    if (!method_context->CallMethod(condition_block, "eval", eval_parameters,
                                    &condition_object)) {
      return nullptr;
    }

    bool condition = false;
    if (!UnwrapBool(method_context, condition_object.object_reference(),
                    &condition)) {
      return nullptr;
    }

    if (!condition) {
      break;
    }

    Value dummy;
    if (!method_context->CallMethod(code_block, "eval", eval_parameters,
                                    &dummy)) {
      return nullptr;
    }
  }

  return MakeNoneObject(method_context);
}

}  // namespace toy_lang
}  // namespace floating_temple
