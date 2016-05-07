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

#include "toy_lang/zoo/symbol_object.h"

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

SymbolObject::SymbolObject() {
}

VersionedLocalObject* SymbolObject::Clone() const {
  return new SymbolObject();
}

void SymbolObject::InvokeMethod(Thread* thread,
                                ObjectReference* object_reference,
                                const string& method_name,
                                const vector<Value>& parameters,
                                Value* return_value) {
  LOG(FATAL) << "Unsupported method: \"" << CEscape(method_name) << "\"";
}

void SymbolObject::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();
  dc->AddString("type");
  dc->AddString("SymbolObject");
  dc->End();
}

void SymbolObject::PopulateObjectProto(ObjectProto* object_proto,
                                       SerializationContext* context) const {
  object_proto->mutable_symbol_object();
}

}  // namespace toy_lang
}  // namespace floating_temple
