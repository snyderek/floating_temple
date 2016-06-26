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

#include "toy_lang/zoo/bool_object.h"

#include <string>
#include <vector>

#include "base/escape.h"
#include "base/logging.h"
#include "toy_lang/proto/serialization.pb.h"
#include "util/dump_context.h"

using std::string;
using std::vector;

namespace floating_temple {
namespace toy_lang {

BoolObject::BoolObject(bool b)
    : b_(b) {
}

LocalObject* BoolObject::Clone() const {
  return new BoolObject(b_);
}

void BoolObject::InvokeMethod(MethodContext* method_context,
                              ObjectReference* self_object_reference,
                              const string& method_name,
                              const vector<Value>& parameters,
                              Value* return_value) {
  CHECK(return_value != nullptr);

  if (method_name == "get_bool") {
    CHECK_EQ(parameters.size(), 0u);
    return_value->set_bool_value(0, b_);
  } else if (method_name == "get_string") {
    CHECK_EQ(parameters.size(), 0u);
    if (b_) {
      return_value->set_string_value(0, "true");
    } else {
      return_value->set_string_value(0, "false");
    }
  } else {
    LOG(FATAL) << "Unsupported method: \"" << CEscape(method_name) << "\"";
  }
}

void BoolObject::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("BoolObject");

  dc->AddString("b");
  dc->AddBool(b_);

  dc->End();
}

// static
BoolObject* BoolObject::ParseBoolProto(const BoolProto& bool_proto) {
  return new BoolObject(bool_proto.value());
}

void BoolObject::PopulateObjectProto(ObjectProto* object_proto,
                                     SerializationContext* context) const {
  object_proto->mutable_bool_object()->set_value(b_);
}

}  // namespace toy_lang
}  // namespace floating_temple
