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

#include "toy_lang/zoo/not_function.h"

#include <vector>

#include "base/logging.h"
#include "toy_lang/proto/serialization.pb.h"
#include "toy_lang/wrap.h"
#include "util/dump_context.h"

using std::vector;

namespace floating_temple {
namespace toy_lang {

NotFunction::NotFunction() {
}

LocalObject* NotFunction::Clone() const {
  return new NotFunction();
}

void NotFunction::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();
  dc->AddString("type");
  dc->AddString("NotFunction");
  dc->End();
}

void NotFunction::PopulateObjectProto(ObjectProto* object_proto,
                                      SerializationContext* context) const {
  object_proto->mutable_not_function();
}

ObjectReference* NotFunction::Call(
    MethodContext* method_context,
    const vector<ObjectReference*>& parameters) const {
  CHECK_EQ(parameters.size(), 1u);

  bool condition = false;
  if (!UnwrapBool(method_context, parameters[0], &condition)) {
    return nullptr;
  }

  return WrapBool(method_context, !condition);
}

}  // namespace toy_lang
}  // namespace floating_temple
