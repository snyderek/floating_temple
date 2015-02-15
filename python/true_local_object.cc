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

#include "python/true_local_object.h"

#include "third_party/Python-3.4.2/Include/Python.h"

#include <cstddef>
#include <string>
#include <vector>

#include "base/logging.h"
#include "python/local_object_impl.h"
#include "python/proto/serialization.pb.h"

using std::string;
using std::vector;

namespace floating_temple {
namespace python {

TrueLocalObject::TrueLocalObject()
    : LocalObjectImpl(Py_True) {
}

LocalObject* TrueLocalObject::Clone() const {
  return new TrueLocalObject();
}

string TrueLocalObject::Dump() const {
  return "{ \"type\": \"TrueLocalObject\" }";
}

void TrueLocalObject::PopulateObjectProto(ObjectProto* object_proto,
                                          SerializationContext* context) const {
  CHECK(object_proto != NULL);
  object_proto->mutable_true_object();
}

bool TrueLocalObject::InvokeTypeSpecificMethod(PeerObject* peer_object,
                                               const string& method_name,
                                               const vector<Value>& parameters,
                                               MethodContext* method_context,
                                               Value* return_value) {
  return false;
}

}  // namespace python
}  // namespace floating_temple