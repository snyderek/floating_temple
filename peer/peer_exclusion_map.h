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

#ifndef PEER_PEER_EXCLUSION_MAP_H_
#define PEER_PEER_EXCLUSION_MAP_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "peer/interval_set.h"
#include "peer/proto/transaction_id.pb.h"
#include "peer/transaction_id_util.h"

namespace floating_temple {
namespace peer {

class CanonicalPeer;
class PeerExclusionMap;

bool PeerExclusionMapsAreEqual(const PeerExclusionMap& a,
                               const PeerExclusionMap& b);

class PeerExclusionMap {
 public:
  PeerExclusionMap();
  ~PeerExclusionMap();

  // The excluded range is [start_transaction_id, end_transaction_id).
  void AddExcludedRange(const CanonicalPeer* origin_peer,
                        const TransactionId& start_transaction_id,
                        const TransactionId& end_transaction_id);

  bool IsTransactionExcluded(const CanonicalPeer* origin_peer,
                             const TransactionId& transaction_id) const;

  void CopyFrom(const PeerExclusionMap& other);
  void Swap(PeerExclusionMap* other);

  std::string Dump() const;

 private:
  std::map<const CanonicalPeer*, IntervalSet<TransactionId> > map_;

  friend bool PeerExclusionMapsAreEqual(const PeerExclusionMap& a,
                                        const PeerExclusionMap& b);

  DISALLOW_COPY_AND_ASSIGN(PeerExclusionMap);
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_PEER_EXCLUSION_MAP_H_
