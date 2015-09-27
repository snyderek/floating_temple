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

#include "toy_lang/zoo/if_function.h"

#include <vector>

#include "base/logging.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "toy_lang/proto/serialization.pb.h"
#include "toy_lang/zoo/none_object.h"
#include "util/dump_context.h"

using std::vector;

namespace floating_temple {
namespace toy_lang {

IfFunction::IfFunction() {
}

VersionedLocalObject* IfFunction::Clone() const {
  return new IfFunction();
}

void IfFunction::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();
  dc->AddString("type");
  dc->AddString("IfFunction");
  dc->End();
}

void IfFunction::PopulateObjectProto(ObjectProto* object_proto,
                                     SerializationContext* context) const {
  object_proto->mutable_if_function();
}

ObjectReference* IfFunction::Call(
    ObjectReference* symbol_table_object, Thread* thread,
    const vector<ObjectReference*>& parameters) const {
  CHECK(thread != nullptr);
  CHECK_GE(parameters.size(), 2u);
  CHECK_LE(parameters.size(), 3u);

  Value condition;
  if (!thread->CallMethod(parameters[0], "get_bool", vector<Value>(),
                          &condition)) {
    return nullptr;
  }

  ObjectReference* expression = nullptr;

  if (condition.bool_value()) {
    expression = parameters[1];
  } else {
    if (parameters.size() < 3u) {
      return thread->CreateVersionedObject(new NoneObject(), "");
    }

    expression = parameters[2];
  }

  vector<Value> eval_parameters(1);
  eval_parameters[0].set_object_reference(0, symbol_table_object);

  Value result;
  if (!thread->CallMethod(expression, "eval", eval_parameters, &result)) {
    return nullptr;
  }

  return result.object_reference();
}

}  // namespace toy_lang
}  // namespace floating_temple