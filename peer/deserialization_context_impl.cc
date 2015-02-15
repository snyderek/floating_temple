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

#include "peer/deserialization_context_impl.h"

#include <vector>

#include "base/logging.h"
#include "peer/peer_object_impl.h"

using std::vector;

namespace floating_temple {
namespace peer {

DeserializationContextImpl::DeserializationContextImpl(
    const vector<PeerObjectImpl*>* peer_objects)
    : peer_objects_(CHECK_NOTNULL(peer_objects)) {
}

PeerObject* DeserializationContextImpl::GetPeerObjectByIndex(int index) {
  CHECK_GE(index, 0);

  const vector<PeerObjectImpl*>::size_type vector_index =
      static_cast<vector<PeerObjectImpl*>::size_type>(index);
  CHECK_LT(vector_index, peer_objects_->size());

  return (*peer_objects_)[vector_index];
}

}  // namespace peer
}  // namespace floating_temple
