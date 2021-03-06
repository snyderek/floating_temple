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

#include "toy_lang/zoo/less_than_function.h"

#include <vector>

#include "base/integral_types.h"
#include "base/logging.h"
#include "toy_lang/proto/serialization.pb.h"
#include "toy_lang/wrap.h"
#include "util/dump_context.h"

using std::vector;

namespace floating_temple {
namespace toy_lang {

LessThanFunction::LessThanFunction() {
}

LocalObject* LessThanFunction::Clone() const {
  return new LessThanFunction();
}

void LessThanFunction::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();
  dc->AddString("type");
  dc->AddString("LessThanFunction");
  dc->End();
}

void LessThanFunction::PopulateObjectProto(
    ObjectProto* object_proto, SerializationContext* context) const {
  object_proto->mutable_less_than_function();
}

ObjectReference* LessThanFunction::Call(
    MethodContext* method_context,
    const vector<ObjectReference*>& parameters) const {
  CHECK_EQ(parameters.size(), 2u);

  int64 operands[2];
  for (int i = 0; i < 2; ++i) {
    if (!UnwrapInt(method_context, parameters[i], &operands[i])) {
      return nullptr;
    }
  }

  return WrapBool(method_context, operands[0] < operands[1]);
}

}  // namespace toy_lang
}  // namespace floating_temple
