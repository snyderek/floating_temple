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

#include "toy_lang/zoo/map_is_set_function.h"

#include <string>
#include <vector>

#include "base/logging.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "toy_lang/proto/serialization.pb.h"
#include "toy_lang/wrap.h"
#include "util/dump_context.h"

using std::string;
using std::vector;

namespace floating_temple {
namespace toy_lang {

MapIsSetFunction::MapIsSetFunction() {
}

LocalObject* MapIsSetFunction::Clone() const {
  return new MapIsSetFunction();
}

void MapIsSetFunction::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();
  dc->AddString("type");
  dc->AddString("MapIsSetFunction");
  dc->End();
}

void MapIsSetFunction::PopulateObjectProto(
    ObjectProto* object_proto, SerializationContext* context) const {
  object_proto->mutable_map_is_set_function();
}

ObjectReference* MapIsSetFunction::Call(
    Thread* thread, const vector<ObjectReference*>& parameters) const {
  CHECK(thread != nullptr);
  CHECK_EQ(parameters.size(), 2u);

  string key;
  if (!UnwrapString(thread, parameters[1], &key)) {
    return nullptr;
  }

  vector<Value> is_set_params(1);
  is_set_params[0].set_string_value(0, key);

  Value result;
  if (!thread->CallMethod(parameters[0], "is_set", is_set_params, &result)) {
    return nullptr;
  }

  return WrapBool(thread, result.bool_value());
}

}  // namespace toy_lang
}  // namespace floating_temple
