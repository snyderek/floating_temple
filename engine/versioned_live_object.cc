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

#include "engine/versioned_live_object.h"

#include <memory>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/mutex.h"
#include "base/mutex_lock.h"
#include "engine/live_object_node.h"
#include "include/c++/versioned_local_object.h"

using std::shared_ptr;
using std::string;
using std::vector;

namespace floating_temple {
namespace peer {

VersionedLiveObject::VersionedLiveObject(VersionedLocalObject* local_object)
    : node_(new LiveObjectNode(local_object)) {
}

VersionedLiveObject::~VersionedLiveObject() {
  LiveObjectNode* const node = GetNode();
  if (node->DecrementRefCount()) {
    delete node;
  }
}

const LocalObject* VersionedLiveObject::local_object() const {
  return GetNode()->local_object();
}

shared_ptr<LiveObject> VersionedLiveObject::Clone() const {
  return shared_ptr<LiveObject>(new VersionedLiveObject(GetNode()));
}

void VersionedLiveObject::Serialize(
    string* data, vector<ObjectReferenceImpl*>* object_references) const {
  GetNode()->Serialize(data, object_references);
}

void VersionedLiveObject::InvokeMethod(Thread* thread,
                                       ObjectReferenceImpl* object_reference,
                                       const string& method_name,
                                       const vector<Value>& parameters,
                                       Value* return_value) {
  LiveObjectNode* const new_node = GetNode()->InvokeMethod(
      thread, object_reference, method_name, parameters, return_value);

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

string VersionedLiveObject::Dump() const {
  return GetNode()->Dump();
}

VersionedLiveObject::VersionedLiveObject(LiveObjectNode* node)
    : node_(CHECK_NOTNULL(node)) {
  node->IncrementRefCount();
}

LiveObjectNode* VersionedLiveObject::GetNode() const {
  MutexLock lock(&node_mu_);
  return node_;
}

}  // namespace peer
}  // namespace floating_temple
