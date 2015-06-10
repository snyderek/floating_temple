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

#include "python/none_local_object.h"

#include "third_party/Python-3.4.2/Include/Python.h"

#include <string>

#include "base/logging.h"
#include "python/proto/serialization.pb.h"
#include "python/versioned_local_object_impl.h"

using std::string;

namespace floating_temple {
namespace python {

NoneLocalObject::NoneLocalObject()
    : VersionedLocalObjectImpl(Py_None) {
}

VersionedLocalObject* NoneLocalObject::Clone() const {
  return new NoneLocalObject();
}

string NoneLocalObject::Dump() const {
  return "{ \"type\": \"NoneLocalObject\" }";
}

void NoneLocalObject::PopulateObjectProto(ObjectProto* object_proto,
                                          SerializationContext* context) const {
  CHECK(object_proto != nullptr);
  object_proto->mutable_py_none_object();
}

}  // namespace python
}  // namespace floating_temple
