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

#include "engine/sequence_point_impl.h"

#include <map>
#include <set>
#include <string>
#include <utility>

#include "base/logging.h"
#include "engine/canonical_peer.h"
#include "engine/max_version_map.h"
#include "engine/peer_exclusion_map.h"
#include "engine/proto/transaction_id.pb.h"
#include "engine/transaction_id_util.h"
#include "util/dump_context.h"

using std::map;
using std::pair;
using std::set;

namespace floating_temple {
namespace engine {

SequencePointImpl::SequencePointImpl() {
}

bool SequencePointImpl::HasPeerTransactionId(
    const CanonicalPeer* canonical_peer,
    const TransactionId& transaction_id) const {
  if (!version_map_.HasPeerTransactionId(canonical_peer, transaction_id) ||
      peer_exclusion_map_.IsTransactionExcluded(canonical_peer,
                                                transaction_id)) {
    return false;
  }

  const map<const CanonicalPeer*, set<TransactionId>>::const_iterator
      rejected_peer_it = rejected_peers_.find(canonical_peer);

  if (rejected_peer_it == rejected_peers_.end()) {
    return true;
  }

  const set<TransactionId>& transaction_ids = rejected_peer_it->second;
  CHECK(!transaction_ids.empty());

  return transaction_id < *transaction_ids.begin();
}

void SequencePointImpl::AddPeerTransactionId(
    const CanonicalPeer* canonical_peer, const TransactionId& transaction_id) {
  version_map_.AddPeerTransactionId(canonical_peer, transaction_id);
}

void SequencePointImpl::AddInvalidatedRange(
    const CanonicalPeer* origin_peer, const TransactionId& start_transaction_id,
    const TransactionId& end_transaction_id) {
  peer_exclusion_map_.AddExcludedRange(origin_peer, start_transaction_id,
                                       end_transaction_id);

  const map<const CanonicalPeer*, set<TransactionId>>::iterator
      rejected_peer_it = rejected_peers_.find(origin_peer);

  if (rejected_peer_it != rejected_peers_.end()) {
    set<TransactionId>* const transaction_ids = &rejected_peer_it->second;

    transaction_ids->erase(transaction_ids->lower_bound(start_transaction_id),
                           transaction_ids->lower_bound(end_transaction_id));

    if (transaction_ids->empty()) {
      rejected_peers_.erase(rejected_peer_it);
    }
  }
}

void SequencePointImpl::AddRejectedPeer(
    const CanonicalPeer* origin_peer,
    const TransactionId& start_transaction_id) {
  rejected_peers_[origin_peer].insert(start_transaction_id);
}

void SequencePointImpl::CopyFrom(const SequencePointImpl& other) {
  version_map_.CopyFrom(other.version_map_);
  peer_exclusion_map_.CopyFrom(other.peer_exclusion_map_);
  rejected_peers_ = other.rejected_peers_;
}

SequencePoint* SequencePointImpl::Clone() const {
  return new SequencePointImpl(version_map_, peer_exclusion_map_,
                               rejected_peers_);
}

void SequencePointImpl::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("version_map");
  version_map_.Dump(dc);

  dc->AddString("peer_exclusion_map");
  peer_exclusion_map_.Dump(dc);

  dc->AddString("rejected_peers");
  dc->BeginMap();
  for (const pair<const CanonicalPeer*, set<TransactionId>>& map_pair :
           rejected_peers_) {
    const CanonicalPeer* const canonical_peer = map_pair.first;
    const set<TransactionId>& rejected_transactions = map_pair.second;

    dc->AddString(canonical_peer->peer_id());
    dc->BeginList();
    for (const TransactionId& transaction_id : rejected_transactions) {
      dc->AddString(TransactionIdToString(transaction_id));
    }
    dc->End();
  }
  dc->End();

  dc->End();
}

SequencePointImpl::SequencePointImpl(
    const MaxVersionMap& version_map,
    const PeerExclusionMap& peer_exclusion_map,
    const map<const CanonicalPeer*, set<TransactionId>>& rejected_peers)
    : version_map_(version_map),
      rejected_peers_(rejected_peers) {
  peer_exclusion_map_.CopyFrom(peer_exclusion_map);
}

}  // namespace engine
}  // namespace floating_temple
