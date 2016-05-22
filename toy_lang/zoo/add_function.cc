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

#include "toy_lang/zoo/add_function.h"

#include <vector>

#include "base/integral_types.h"
#include "base/logging.h"
#include "toy_lang/proto/serialization.pb.h"
#include "toy_lang/wrap.h"
#include "util/dump_context.h"

using std::vector;

namespace floating_temple {
namespace toy_lang {

AddFunction::AddFunction() {
}

VersionedLocalObject* AddFunction::Clone() const {
  return new AddFunction();
}

void AddFunction::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();
  dc->AddString("type");
  dc->AddString("AddFunction");
  dc->End();
}

void AddFunction::PopulateObjectProto(ObjectProto* object_proto,
                                      SerializationContext* context) const {
  object_proto->mutable_add_function();
}

ObjectReference* AddFunction::Call(
    Thread* thread, const vector<ObjectReference*>& parameters) const {
  int64 sum = 0;

  for (ObjectReference* const object_reference : parameters) {
    int64 number = 0;
    if (!UnwrapInt(thread, object_reference, &number)) {
      return nullptr;
    }

    sum += number;
  }

  return WrapInt(thread, sum);
}

}  // namespace toy_lang
}  // namespace floating_temple
