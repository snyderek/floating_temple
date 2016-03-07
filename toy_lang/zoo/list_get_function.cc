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

#include "toy_lang/zoo/list_get_function.h"

#include <vector>

#include "base/logging.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "toy_lang/proto/serialization.pb.h"
#include "util/dump_context.h"

using std::vector;

namespace floating_temple {
namespace toy_lang {

ListGetFunction::ListGetFunction() {
}

VersionedLocalObject* ListGetFunction::Clone() const {
  return new ListGetFunction();
}

void ListGetFunction::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();
  dc->AddString("type");
  dc->AddString("ListGetFunction");
  dc->End();
}

void ListGetFunction::PopulateObjectProto(ObjectProto* object_proto,
                                          SerializationContext* context) const {
  object_proto->mutable_list_get_function();
}

ObjectReference* ListGetFunction::Call(
    ObjectReference* symbol_table_object, Thread* thread,
    const vector<ObjectReference*>& parameters) const {
  CHECK(thread != nullptr);
  CHECK_EQ(parameters.size(), 2u);

  Value index;
  if (!thread->CallMethod(parameters[1], "get_int", vector<Value>(), &index)) {
    return nullptr;
  }

  vector<Value> get_at_params(1);
  get_at_params[0] = index;

  Value item;
  if (!thread->CallMethod(parameters[0], "get_at", get_at_params, &item)) {
    return nullptr;
  }

  return item.object_reference();
}

}  // namespace toy_lang
}  // namespace floating_temple
