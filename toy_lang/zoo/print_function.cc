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

#include "toy_lang/zoo/print_function.h"

#include <cstdio>
#include <vector>

#include "base/logging.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "toy_lang/proto/serialization.pb.h"
#include "toy_lang/zoo/none_object.h"
#include "util/dump_context.h"

using std::printf;
using std::vector;

namespace floating_temple {
namespace toy_lang {

PrintFunction::PrintFunction() {
}

VersionedLocalObject* PrintFunction::Clone() const {
  return new PrintFunction();
}

void PrintFunction::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();
  dc->AddString("type");
  dc->AddString("PrintFunction");
  dc->End();
}

void PrintFunction::PopulateObjectProto(ObjectProto* object_proto,
                                        SerializationContext* context) const {
  object_proto->mutable_print_function();
}

ObjectReference* PrintFunction::Call(
    ObjectReference* symbol_table_object, Thread* thread,
    const vector<ObjectReference*>& parameters) const {
  CHECK(thread != nullptr);

  for (vector<ObjectReference*>::const_iterator it = parameters.begin();
       it != parameters.end(); ++it) {
    if (it != parameters.begin()) {
      printf(" ");
    }

    Value str;
    if (!thread->CallMethod(*it, "get_string", vector<Value>(), &str)) {
      return nullptr;
    }

    printf("%s", str.string_value().c_str());
  }

  printf("\n");

  return thread->CreateVersionedObject(new NoneObject(), "");
}

}  // namespace toy_lang
}  // namespace floating_temple
