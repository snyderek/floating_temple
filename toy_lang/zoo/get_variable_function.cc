// Floating Temple
// Copyright 2016 Derek S. Snyder
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

#include "toy_lang/zoo/get_variable_function.h"

#include <vector>

#include "base/logging.h"
#include "include/c++/method_context.h"
#include "include/c++/value.h"
#include "toy_lang/proto/serialization.pb.h"
#include "util/dump_context.h"

using std::vector;

namespace floating_temple {
namespace toy_lang {

GetVariableFunction::GetVariableFunction() {
}

LocalObject* GetVariableFunction::Clone() const {
  return new GetVariableFunction();
}

void GetVariableFunction::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();
  dc->AddString("type");
  dc->AddString("GetVariableFunction");
  dc->End();
}

void GetVariableFunction::PopulateObjectProto(
    ObjectProto* object_proto, SerializationContext* context) const {
  object_proto->mutable_get_variable_function();
}

ObjectReference* GetVariableFunction::Call(
    MethodContext* method_context,
    const vector<ObjectReference*>& parameters) const {
  CHECK(method_context != nullptr);
  CHECK_EQ(parameters.size(), 1u);

  Value value;
  if (!method_context->CallMethod(parameters[0], "get", vector<Value>(),
                                  &value)) {
    return nullptr;
  }

  return value.object_reference();
}

}  // namespace toy_lang
}  // namespace floating_temple
