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

#include "peer/sequence_point_impl.h"

#include <map>
#include <set>
#include <string>

#include "base/string_printf.h"
#include "peer/canonical_peer.h"
#include "peer/max_version_map.h"
#include "peer/peer_exclusion_map.h"
#include "peer/proto/transaction_id.pb.h"
#include "peer/transaction_id_util.h"

using std::map;
using std::set;
using std::string;

namespace floating_temple {
namespace peer {

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

  return CompareTransactionIds(transaction_id, *transaction_ids.begin()) < 0;
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

string SequencePointImpl::Dump() const {
  string rejected_peers_string;

  if (rejected_peers_.empty()) {
    rejected_peers_string = "{}";
  } else {
    rejected_peers_string = "{";

    for (map<const CanonicalPeer*, set<TransactionId>>::const_iterator it1 =
             rejected_peers_.begin();
         it1 != rejected_peers_.end(); ++it1) {
      const CanonicalPeer* const canonical_peer = it1->first;
      const set<TransactionId>& rejected_transactions = it1->second;

      if (it1 != rejected_peers_.begin()) {
        rejected_peers_string += ",";
      }

      string rejected_transactions_string;

      if (rejected_transactions.empty()) {
        rejected_transactions_string = "[]";
      } else {
        rejected_transactions_string = "[";

        for (set<TransactionId>::const_iterator it2 =
                 rejected_transactions.begin();
             it2 != rejected_transactions.end(); ++it2) {
          if (it2 != rejected_transactions.begin()) {
            rejected_transactions_string += ",";
          }

          StringAppendF(&rejected_transactions_string, " \"%s\"",
                        TransactionIdToString(*it2).c_str());
        }

        rejected_transactions_string += " ]";
      }

      StringAppendF(&rejected_peers_string, " \"%s\": %s",
                    CEscape(canonical_peer->peer_id()).c_str(),
                    rejected_transactions_string.c_str());
    }

    rejected_peers_string += " }";
  }

  return StringPrintf(
      "{ \"version_map\": %s, \"peer_exclusion_map\": %s, "
      "\"rejected_peers\": %s }",
      version_map_.Dump().c_str(), peer_exclusion_map_.Dump().c_str(),
      rejected_peers_string.c_str());
}

SequencePointImpl::SequencePointImpl(
    const MaxVersionMap& version_map,
    const PeerExclusionMap& peer_exclusion_map,
    const map<const CanonicalPeer*, set<TransactionId>>& rejected_peers)
    : version_map_(version_map),
      rejected_peers_(rejected_peers) {
  peer_exclusion_map_.CopyFrom(peer_exclusion_map);
}

}  // namespace peer
}  // namespace floating_temple
