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

#include "toy_lang/zoo/int_object.h"

#include <cinttypes>
#include <string>
#include <vector>

#include "base/escape.h"
#include "base/integral_types.h"
#include "base/logging.h"
#include "base/string_printf.h"
#include "toy_lang/proto/serialization.pb.h"
#include "util/dump_context.h"

using std::string;
using std::vector;

namespace floating_temple {
namespace toy_lang {

IntObject::IntObject(int64 n)
    : n_(n) {
}

VersionedLocalObject* IntObject::Clone() const {
  return new IntObject(n_);
}

void IntObject::InvokeMethod(Thread* thread,
                             ObjectReference* object_reference,
                             const string& method_name,
                             const vector<Value>& parameters,
                             Value* return_value) {
  CHECK(return_value != nullptr);

  if (method_name == "get_int") {
    CHECK_EQ(parameters.size(), 0u);
    return_value->set_int64_value(0, n_);
  } else if (method_name == "get_string") {
    CHECK_EQ(parameters.size(), 0u);
    return_value->set_string_value(0, StringPrintf("%" PRId64, n_));
  } else {
    LOG(FATAL) << "Unsupported method: \"" << CEscape(method_name) << "\"";
  }
}

void IntObject::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("IntObject");

  dc->AddString("n");
  dc->AddInt64(n_);

  dc->End();
}

// static
IntObject* IntObject::ParseIntProto(const IntProto& int_proto) {
  return new IntObject(int_proto.value());
}

void IntObject::PopulateObjectProto(ObjectProto* object_proto,
                                    SerializationContext* context) const {
  object_proto->mutable_int_object()->set_value(n_);
}

}  // namespace toy_lang
}  // namespace floating_temple
