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

#include "engine/canonical_peer_map.h"

#include <memory>
#include <string>
#include <unordered_map>

#include "base/mutex.h"
#include "base/mutex_lock.h"
#include "engine/canonical_peer.h"

using std::string;
using std::unique_ptr;

namespace floating_temple {
namespace engine {

CanonicalPeerMap::CanonicalPeerMap() {
}

CanonicalPeerMap::~CanonicalPeerMap() {
}

const CanonicalPeer* CanonicalPeerMap::GetCanonicalPeer(const string& peer_id) {
  MutexLock lock(&mu_);

  unique_ptr<CanonicalPeer>& canonical_peer = map_[peer_id];

  if (canonical_peer.get() == nullptr) {
    canonical_peer.reset(new CanonicalPeer(peer_id));
  }

  return canonical_peer.get();
}

}  // namespace engine
}  // namespace floating_temple
