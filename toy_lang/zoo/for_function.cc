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
  CHECK_EQ(parameters.size(), 3u);

  Value variable_name;
  if (!thread->CallMethod(parameters[0], "get_string", vector<Value>(),
                          &variable_name)) {
    return nullptr;
  }

  ObjectReference* const iter = parameters[1];
  ObjectReference* const expression = parameters[2];

  const vector<Value> eval_parameters;

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

    // TODO(dss): Set the loop variable.

    Value dummy;
    if (!thread->CallMethod(expression, "eval", eval_parameters, &dummy)) {
      return nullptr;
    }
  }

  return thread->CreateVersionedObject(new NoneObject(), "");
}

}  // namespace toy_lang
}  // namespace floating_temple
