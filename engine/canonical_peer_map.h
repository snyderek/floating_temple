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

#ifndef ENGINE_CANONICAL_PEER_MAP_H_
#define ENGINE_CANONICAL_PEER_MAP_H_

#include <string>
#include <unordered_map>

#include "base/linked_ptr.h"
#include "base/macros.h"
#include "base/mutex.h"

namespace floating_temple {
namespace peer {

class CanonicalPeer;

class CanonicalPeerMap {
 public:
  CanonicalPeerMap();
  ~CanonicalPeerMap();

  const CanonicalPeer* GetCanonicalPeer(const std::string& peer_id);

 private:
  // TODO(dss): Delete CanonicalPeer instances when they're no longer being
  // used.
  std::unordered_map<std::string, linked_ptr<CanonicalPeer>> map_;
  mutable Mutex mu_;

  DISALLOW_COPY_AND_ASSIGN(CanonicalPeerMap);
};

}  // namespace peer
}  // namespace floating_temple

#endif  // ENGINE_CANONICAL_PEER_MAP_H_
