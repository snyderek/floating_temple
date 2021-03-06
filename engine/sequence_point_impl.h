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

#ifndef ENGINE_SEQUENCE_POINT_IMPL_H_
#define ENGINE_SEQUENCE_POINT_IMPL_H_

#include <map>
#include <set>

#include "base/macros.h"
#include "engine/max_version_map.h"
#include "engine/peer_exclusion_map.h"
#include "engine/proto/transaction_id.pb.h"
#include "engine/sequence_point.h"
#include "engine/transaction_id_util.h"

namespace floating_temple {
namespace engine {

class CanonicalPeer;

class SequencePointImpl : public SequencePoint {
 public:
  SequencePointImpl();

  const MaxVersionMap& version_map() const { return version_map_; }
  const PeerExclusionMap& peer_exclusion_map() const
      { return peer_exclusion_map_; }
  const std::map<const CanonicalPeer*, std::set<TransactionId>>&
      rejected_peers() const { return rejected_peers_; }

  bool HasPeerTransactionId(const CanonicalPeer* canonical_peer,
                            const TransactionId& transaction_id) const;

  void AddPeerTransactionId(const CanonicalPeer* canonical_peer,
                            const TransactionId& transaction_id);
  void AddInvalidatedRange(const CanonicalPeer* origin_peer,
                           const TransactionId& start_transaction_id,
                           const TransactionId& end_transaction_id);
  void AddRejectedPeer(const CanonicalPeer* origin_peer,
                       const TransactionId& start_transaction_id);

  void CopyFrom(const SequencePointImpl& other);

  SequencePoint* Clone() const override;
  void Dump(DumpContext* dc) const override;

 private:
  SequencePointImpl(
      const MaxVersionMap& version_map,
      const PeerExclusionMap& peer_exclusion_map,
      const std::map<const CanonicalPeer*, std::set<TransactionId>>&
          rejected_peers);

  MaxVersionMap version_map_;
  PeerExclusionMap peer_exclusion_map_;
  std::map<const CanonicalPeer*, std::set<TransactionId>> rejected_peers_;

  DISALLOW_COPY_AND_ASSIGN(SequencePointImpl);
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_SEQUENCE_POINT_IMPL_H_
