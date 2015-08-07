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

#include "engine/peer_exclusion_map.h"

#include <map>
#include <utility>
#include <vector>

#include "base/escape.h"
#include "base/logging.h"
#include "engine/canonical_peer.h"
#include "engine/interval_set.h"
#include "engine/proto/transaction_id.pb.h"
#include "engine/transaction_id_util.h"
#include "util/dump_context.h"

using std::map;
using std::pair;
using std::vector;

namespace floating_temple {
namespace engine {

bool PeerExclusionMapsAreEqual(const PeerExclusionMap& a,
                               const PeerExclusionMap& b) {
  return a.map_ == b.map_;
}

PeerExclusionMap::PeerExclusionMap() {
}

PeerExclusionMap::~PeerExclusionMap() {
}

void PeerExclusionMap::AddExcludedRange(
    const CanonicalPeer* origin_peer, const TransactionId& start_transaction_id,
    const TransactionId& end_transaction_id) {
  CHECK(origin_peer != nullptr);

  map_[origin_peer].AddInterval(start_transaction_id, end_transaction_id);
}

bool PeerExclusionMap::IsTransactionExcluded(
    const CanonicalPeer* origin_peer,
    const TransactionId& transaction_id) const {
  CHECK(origin_peer != nullptr);

  const map<const CanonicalPeer*, IntervalSet<TransactionId>>::const_iterator
      it = map_.find(origin_peer);

  if (it == map_.end()) {
    return false;
  }

  return it->second.Contains(transaction_id);
}

void PeerExclusionMap::CopyFrom(const PeerExclusionMap& other) {
  map_ = other.map_;
}

void PeerExclusionMap::Swap(PeerExclusionMap* other) {
  map_.swap(other->map_);
}

void PeerExclusionMap::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  for (const pair<const CanonicalPeer*, IntervalSet<TransactionId>>& map_pair :
           map_) {
    const CanonicalPeer* const canonical_peer = map_pair.first;
    const IntervalSet<TransactionId>& interval_set = map_pair.second;

    dc->AddString(canonical_peer->peer_id());

    vector<TransactionId> end_points;
    interval_set.GetEndPoints(&end_points);

    dc->BeginList();
    for (vector<TransactionId>::const_iterator it = end_points.begin();
         it != end_points.end(); ++it) {
      dc->BeginList();
      dc->AddString(TransactionIdToString(*it));
      ++it;
      CHECK(it != end_points.end());
      dc->AddString(TransactionIdToString(*it));
      dc->End();
    }
    dc->End();
  }

  dc->End();
}

}  // namespace engine
}  // namespace floating_temple
