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

#ifndef ENGINE_CANONICAL_PEER_H_
#define ENGINE_CANONICAL_PEER_H_

#include <string>

#include "base/macros.h"

namespace floating_temple {
namespace engine {

class CanonicalPeer {
 public:
  explicit CanonicalPeer(const std::string& peer_id);
  ~CanonicalPeer();

  const std::string& peer_id() const { return peer_id_; }

 private:
  const std::string peer_id_;

  DISALLOW_COPY_AND_ASSIGN(CanonicalPeer);
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_CANONICAL_PEER_H_
