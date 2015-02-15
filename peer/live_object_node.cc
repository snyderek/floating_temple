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

#include "peer/live_object_node.h"

#include <string>
#include <vector>

#include "base/escape.h"
#include "base/logging.h"
#include "base/mutex.h"
#include "base/mutex_lock.h"
#include "include/c++/local_object.h"
#include "include/c++/value.h"
#include "peer/peer_object_impl.h"
#include "peer/serialize_local_object_to_string.h"

using std::string;
using std::vector;

namespace floating_temple {
namespace peer {

LiveObjectNode::LiveObjectNode(LocalObject* local_object)
    : local_object_(CHECK_NOTNULL(local_object)),
      ref_count_(1) {
}

LiveObjectNode::~LiveObjectNode() {
  delete local_object_;
}

void LiveObjectNode::Serialize(
    string* data, vector<PeerObjectImpl*>* referenced_peer_objects) const {
  SerializeLocalObjectToString(local_object_, data, referenced_peer_objects);
}

LiveObjectNode* LiveObjectNode::InvokeMethod(Thread* thread,
                                             PeerObjectImpl* peer_object,
                                             const string& method_name,
                                             const vector<Value>& parameters,
                                             Value* return_value) {
  const int ref_count = GetRefCount();
  CHECK_GE(ref_count, 1);

  VLOG(2) << "Method: \"" << CEscape(method_name) << "\"";

  if (ref_count > 1) {
    LocalObject* const new_local_object = local_object_->Clone();
    VLOG(4) << "Before: " << new_local_object->Dump();
    new_local_object->InvokeMethod(thread, peer_object, method_name, parameters,
                                   return_value);
    VLOG(4) << "After: " << new_local_object->Dump();

    return new LiveObjectNode(new_local_object);
  } else {
    VLOG(4) << "Before: " << local_object_->Dump();
    local_object_->InvokeMethod(thread, peer_object, method_name, parameters,
                                return_value);
    VLOG(4) << "After: " << local_object_->Dump();

    return this;
  }
}

string LiveObjectNode::Dump() const {
  return local_object_->Dump();
}

void LiveObjectNode::IncrementRefCount() {
  MutexLock lock(&ref_count_mu_);
  ++ref_count_;
  CHECK_GE(ref_count_, 2);
}

bool LiveObjectNode::DecrementRefCount() {
  MutexLock lock(&ref_count_mu_);
  CHECK_GE(ref_count_, 1);
  --ref_count_;
  return ref_count_ == 0;
}

int LiveObjectNode::GetRefCount() const {
  MutexLock lock(&ref_count_mu_);
  return ref_count_;
}

}  // namespace peer
}  // namespace floating_temple
