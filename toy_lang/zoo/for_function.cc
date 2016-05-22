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

#include "toy_lang/zoo/for_function.h"

#include <string>
#include <vector>

#include "base/logging.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "toy_lang/proto/serialization.pb.h"
#include "toy_lang/zoo/list_object.h"
#include "toy_lang/zoo/none_object.h"
#include "toy_lang/zoo/variable_object.h"
#include "util/dump_context.h"

using std::vector;

namespace floating_temple {

class ObjectReference;

namespace toy_lang {

ForFunction::ForFunction() {
}

VersionedLocalObject* ForFunction::Clone() const {
  return new ForFunction();
}

void ForFunction::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();
  dc->AddString("type");
  dc->AddString("ForFunction");
  dc->End();
}

void ForFunction::PopulateObjectProto(ObjectProto* object_proto,
                                      SerializationContext* context) const {
  object_proto->mutable_for_function();
}

ObjectReference* ForFunction::Call(
    Thread* thread, const vector<ObjectReference*>& parameters) const {
  CHECK(thread != nullptr);
  CHECK_EQ(parameters.size(), 2u);

  ObjectReference* const iter = parameters[0];
  ObjectReference* const code_block = parameters[1];

  for (;;) {
    Value has_next;
    if (!thread->CallMethod(iter, "has_next", vector<Value>(), &has_next)) {
      return nullptr;
    }

    if (!has_next.bool_value()) {
      break;
    }

    Value iter_value;
    if (!thread->CallMethod(iter, "get_next", vector<Value>(), &iter_value)) {
      return nullptr;
    }

    ObjectReference* const iter_variable = thread->CreateVersionedObject(
        new VariableObject(iter_value.object_reference()), "");
    const vector<ObjectReference*> block_parameters(1, iter_variable);

    vector<Value> eval_parameters(1);
    eval_parameters[0].set_object_reference(
        0, thread->CreateVersionedObject(new ListObject(block_parameters), ""));

    Value dummy;
    if (!thread->CallMethod(code_block, "eval", eval_parameters, &dummy)) {
      return nullptr;
    }
  }

  return thread->CreateVersionedObject(new NoneObject(), "");
}

}  // namespace toy_lang
}  // namespace floating_temple
