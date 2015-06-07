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

#include "peer/unversioned_live_object.h"

#include <memory>
#include <string>
#include <vector>

#include "base/logging.h"
#include "include/c++/unversioned_local_object.h"
#include "peer/peer_object_impl.h"

using std::shared_ptr;
using std::string;
using std::vector;

namespace floating_temple {
namespace peer {

UnversionedLiveObject::UnversionedLiveObject(
    UnversionedLocalObject* local_object)
    : local_object_(CHECK_NOTNULL(local_object)) {
}

UnversionedLiveObject::~UnversionedLiveObject() {
}

const LocalObject* UnversionedLiveObject::local_object() const {
  return local_object_;
}

shared_ptr<LiveObject> UnversionedLiveObject::Clone() const {
  return shared_ptr<LiveObject>(new UnversionedLiveObject(local_object_));
}

void UnversionedLiveObject::Serialize(
    string* data, vector<PeerObjectImpl*>* referenced_peer_objects) const {
  LOG(FATAL) << "UnversionedLiveObject::Serialize should never be called.";
}

void UnversionedLiveObject::InvokeMethod(Thread* thread,
                                         PeerObjectImpl* peer_object,
                                         const string& method_name,
                                         const vector<Value>& parameters,
                                         Value* return_value) {
  local_object_->InvokeMethod(thread, peer_object, method_name, parameters,
                              return_value);
}

string UnversionedLiveObject::Dump() const {
  return local_object_->Dump();
}

}  // namespace peer
}  // namespace floating_temple
