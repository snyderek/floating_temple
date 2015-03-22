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

#include "peer/peer_exclusion_map.h"

#include <map>
#include <string>
#include <vector>

#include "base/escape.h"
#include "base/logging.h"
#include "base/string_printf.h"
#include "peer/canonical_peer.h"
#include "peer/interval_set.h"
#include "peer/proto/transaction_id.pb.h"
#include "peer/transaction_id_util.h"

using std::map;
using std::string;
using std::vector;

namespace floating_temple {
namespace peer {

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

string PeerExclusionMap::Dump() const {
  string exclusion_map_string;

  if (map_.empty()) {
    exclusion_map_string = "{}";
  } else {
    exclusion_map_string = "{";

    for (map<const CanonicalPeer*, IntervalSet<TransactionId>>::const_iterator
             it1 = map_.begin();
         it1 != map_.end(); ++it1) {
      const CanonicalPeer* const canonical_peer = it1->first;
      const IntervalSet<TransactionId>& interval_set = it1->second;

      if (it1 != map_.begin()) {
        exclusion_map_string += ",";
      }

      vector<TransactionId> end_points;
      interval_set.GetEndPoints(&end_points);

      string interval_set_string;

      if (end_points.empty()) {
        interval_set_string = "[]";
      } else {
        interval_set_string = "[";

        for (vector<TransactionId>::const_iterator it2 = end_points.begin();
             it2 != end_points.end(); ++it2) {
          if (it2 != end_points.begin()) {
            interval_set_string += ",";
          }

          StringAppendF(&interval_set_string, " [ \"%s\", ",
                        TransactionIdToString(*it2).c_str());

          ++it2;
          CHECK(it2 != end_points.end());

          StringAppendF(&interval_set_string, "\"%s\" ]",
                        TransactionIdToString(*it2).c_str());
        }

        interval_set_string += " ]";
      }

      StringAppendF(&exclusion_map_string, " \"%s\": %s",
                    CEscape(canonical_peer->peer_id()).c_str(),
                    interval_set_string.c_str());
    }

    exclusion_map_string += " }";
  }

  return exclusion_map_string;
}

}  // namespace peer
}  // namespace floating_temple
