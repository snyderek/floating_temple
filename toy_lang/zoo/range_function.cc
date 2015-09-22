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

#include "toy_lang/zoo/range_function.h"

#include <vector>

#include "base/logging.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "toy_lang/proto/serialization.pb.h"
#include "toy_lang/zoo/range_iterator_object.h"
#include "util/dump_context.h"

using std::vector;

namespace floating_temple {
namespace toy_lang {

RangeFunction::RangeFunction() {
}

VersionedLocalObject* RangeFunction::Clone() const {
  return new RangeFunction();
}

void RangeFunction::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();
  dc->AddString("type");
  dc->AddString("RangeFunction");
  dc->End();
}

void RangeFunction::PopulateObjectProto(ObjectProto* object_proto,
                                        SerializationContext* context) const {
  object_proto->mutable_range_function();
}

ObjectReference* RangeFunction::Call(
    ObjectReference* symbol_table_object, Thread* thread,
    const vector<ObjectReference*>& parameters) const {
  CHECK(thread != nullptr);
  CHECK_EQ(parameters.size(), 1u);

  Value limit;
  if (!thread->CallMethod(parameters[0], "get_int", vector<Value>(), &limit)) {
    return nullptr;
  }

  return thread->CreateVersionedObject(
      new RangeIteratorObject(limit.int64_value(), 0), "");
}

}  // namespace toy_lang
}  // namespace floating_temple
