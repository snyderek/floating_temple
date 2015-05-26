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

#include "peer/live_object.h"

#include <memory>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/mutex.h"
#include "base/mutex_lock.h"
#include "include/c++/value.h"
#include "peer/live_object_node.h"

using std::shared_ptr;
using std::string;
using std::vector;

namespace floating_temple {
namespace peer {

LiveObject::LiveObject(VersionedLocalObject* local_object)
    : node_(new LiveObjectNode(local_object)) {
}

LiveObject::~LiveObject() {
  LiveObjectNode* const node = GetNode();
  if (node->DecrementRefCount()) {
    delete node;
  }
}

const VersionedLocalObject* LiveObject::local_object() const {
  return GetNode()->local_object();
}

shared_ptr<LiveObject> LiveObject::Clone() const {
  return shared_ptr<LiveObject>(new LiveObject(GetNode()));
}

void LiveObject::Serialize(
    string* data, vector<PeerObjectImpl*>* referenced_peer_objects) const {
  GetNode()->Serialize(data, referenced_peer_objects);
}

void LiveObject::InvokeMethod(Thread* thread,
                              PeerObjectImpl* peer_object,
                              const string& method_name,
                              const vector<Value>& parameters,
                              Value* return_value) {
  LiveObjectNode* const new_node = GetNode()->InvokeMethod(
      thread, peer_object, method_name, parameters, return_value);

  LiveObjectNode* old_node = nullptr;
  {
    MutexLock lock(&node_mu_);
    if (new_node != node_) {
      old_node = node_;
      node_ = new_node;
    }
  }

  if (old_node != nullptr) {
    if (old_node->DecrementRefCount()) {
      delete old_node;
    }
  }
}

string LiveObject::Dump() const {
  return GetNode()->Dump();
}

LiveObject::LiveObject(LiveObjectNode* node)
    : node_(CHECK_NOTNULL(node)) {
  node->IncrementRefCount();
}

LiveObjectNode* LiveObject::GetNode() const {
  MutexLock lock(&node_mu_);
  return node_;
}

}  // namespace peer
}  // namespace floating_temple
