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

#ifndef PEER_SHARED_OBJECT_TRANSACTION_INFO_H_
#define PEER_SHARED_OBJECT_TRANSACTION_INFO_H_

#include <vector>

#include "base/linked_ptr.h"

namespace floating_temple {
namespace peer {

class CanonicalPeer;
class CommittedEvent;

// TODO(dss): Rename this struct to better distinguish it from the
// SharedObjectTransaction class.
struct SharedObjectTransactionInfo {
  std::vector<linked_ptr<CommittedEvent> > events;
  const CanonicalPeer* origin_peer;
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_SHARED_OBJECT_TRANSACTION_INFO_H_
