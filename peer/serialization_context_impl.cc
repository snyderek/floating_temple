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

#include "peer/serialization_context_impl.h"

#include <cstddef>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "peer/peer_object_impl.h"

using std::make_pair;
using std::pair;
using std::unordered_map;
using std::vector;

namespace floating_temple {
namespace peer {

SerializationContextImpl::SerializationContextImpl(
    vector<PeerObjectImpl*>* peer_objects)
    : peer_objects_(CHECK_NOTNULL(peer_objects)) {
}

int SerializationContextImpl::GetIndexForPeerObject(PeerObject* peer_object) {
  CHECK(peer_object != NULL);

  PeerObjectImpl* const peer_object_impl = static_cast<PeerObjectImpl*>(
      peer_object);

  const pair<unordered_map<PeerObjectImpl*, int>::iterator, bool>
      insert_result = indexes_.insert(make_pair(peer_object_impl, -1));

  if (insert_result.second) {
    const int new_index = static_cast<int>(peer_objects_->size());
    peer_objects_->push_back(peer_object_impl);
    insert_result.first->second = new_index;
  }

  return insert_result.first->second;
}

}  // namespace peer
}  // namespace floating_temple
