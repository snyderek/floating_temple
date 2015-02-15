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

#include "peer/canonical_peer.h"

#include <string>

#include "base/logging.h"

using std::string;

namespace floating_temple {
namespace peer {

CanonicalPeer::CanonicalPeer(const string& peer_id)
    : peer_id_(peer_id) {
  CHECK(!peer_id.empty());
}

CanonicalPeer::~CanonicalPeer() {
}

}  // namespace peer
}  // namespace floating_temple
