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

#include "toy_lang/zoo/set_variable_function.h"

#include <vector>

#include "base/logging.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "toy_lang/proto/serialization.pb.h"
#include "toy_lang/wrap.h"
#include "util/dump_context.h"

using std::vector;

namespace floating_temple {
namespace toy_lang {

SetVariableFunction::SetVariableFunction() {
}

VersionedLocalObject* SetVariableFunction::Clone() const {
  return new SetVariableFunction();
}

void SetVariableFunction::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();
  dc->AddString("type");
  dc->AddString("SetVariableFunction");
  dc->End();
}

void SetVariableFunction::PopulateObjectProto(
    ObjectProto* object_proto, SerializationContext* context) const {
  object_proto->mutable_set_variable_function();
}

ObjectReference* SetVariableFunction::Call(
    Thread* thread, const vector<ObjectReference*>& parameters) const {
  CHECK(thread != nullptr);
  CHECK_EQ(parameters.size(), 2u);

  vector<Value> set_parameters(1);
  set_parameters[0].set_object_reference(0, parameters[1]);

  Value dummy;
  if (!thread->CallMethod(parameters[0], "set", set_parameters, &dummy)) {
    return nullptr;
  }

  return MakeNoneObject(thread);
}

}  // namespace toy_lang
}  // namespace floating_temple
