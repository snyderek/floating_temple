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

#include "peer/versioned_object_content.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/escape.h"
#include "base/linked_ptr.h"
#include "base/logging.h"
#include "base/mutex.h"
#include "base/mutex_lock.h"
#include "base/string_printf.h"
#include "peer/canonical_peer.h"
#include "peer/committed_event.h"
#include "peer/live_object.h"
#include "peer/max_version_map.h"
#include "peer/peer_exclusion_map.h"
#include "peer/peer_thread.h"
#include "peer/proto/transaction_id.pb.h"
#include "peer/sequence_point_impl.h"
#include "peer/shared_object_transaction.h"
#include "peer/shared_object_transaction_info.h"
#include "peer/transaction_id_util.h"
#include "peer/transaction_store_internal_interface.h"

using std::map;
using std::pair;
using std::shared_ptr;
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

namespace floating_temple {
namespace peer {
namespace {

vector<pair<const CanonicalPeer*, TransactionId>>::const_iterator
FindTransactionIdInVector(
    const vector<pair<const CanonicalPeer*, TransactionId>>& transaction_pairs,
    const TransactionId& transaction_id) {
  const vector<pair<const CanonicalPeer*, TransactionId>>::const_iterator
      end_it = transaction_pairs.end();
  vector<pair<const CanonicalPeer*, TransactionId>>::const_iterator it =
      transaction_pairs.begin();

  while (it != end_it &&
         CompareTransactionIds(it->second, transaction_id) != 0) {
    ++it;
  }

  return it;
}

}  // namespace

VersionedObjectContent::VersionedObjectContent(
    TransactionStoreInternalInterface* transaction_store,
    SharedObject* shared_object)
    : transaction_store_(CHECK_NOTNULL(transaction_store)),
      shared_object_(CHECK_NOTNULL(shared_object)) {
}

VersionedObjectContent::~VersionedObjectContent() {
}

shared_ptr<const LiveObject> VersionedObjectContent::GetWorkingVersion(
    const MaxVersionMap& transaction_store_version_map,
    const SequencePointImpl& sequence_point,
    unordered_map<SharedObject*, PeerObjectImpl*>* new_peer_objects,
    vector<pair<const CanonicalPeer*, TransactionId>>* transactions_to_reject) {
  MutexLock lock(&committed_versions_mu_);

  MaxVersionMap effective_version;
  ComputeEffectiveVersion_Locked(transaction_store_version_map,
                                 &effective_version);

  if (!VersionMapIsLessThanOrEqual(sequence_point.version_map(),
                                   effective_version)) {
    VLOG(1) << "sequence_point.version_map() == "
            << sequence_point.version_map().Dump();
    VLOG(1) << "effective_version == " << effective_version.Dump();

    return shared_ptr<const LiveObject>(nullptr);
  }

  if (CanUseCachedLiveObject_Locked(sequence_point)) {
    CHECK(cached_live_object_.get() != nullptr);
    return cached_live_object_;
  }

  for (;;) {
    PeerThread peer_thread;
    peer_thread.Start(transaction_store_, shared_object_,
                      shared_ptr<LiveObject>(nullptr), new_peer_objects);

    const bool success = ApplyTransactionsToWorkingVersion_Locked(
        &peer_thread, sequence_point, transactions_to_reject);

    peer_thread.Stop();

    if (success) {
      return peer_thread.live_object();
    }
  }

  return shared_ptr<const LiveObject>(nullptr);
}

void VersionedObjectContent::GetTransactions(
    const MaxVersionMap& transaction_store_version_map,
    map<TransactionId, linked_ptr<SharedObjectTransactionInfo>>* transactions,
    MaxVersionMap* effective_version) const {
  CHECK(transactions != nullptr);

  MutexLock lock(&committed_versions_mu_);

  for (const auto& transaction_pair : committed_versions_) {
    const TransactionId& transaction_id = transaction_pair.first;
    const SharedObjectTransaction& transaction = *transaction_pair.second;

    SharedObjectTransactionInfo* const transaction_info =
        new SharedObjectTransactionInfo();

    const vector<linked_ptr<CommittedEvent>>& src_events = transaction.events();
    vector<linked_ptr<CommittedEvent>>* const dest_events =
        &transaction_info->events;
    const vector<linked_ptr<CommittedEvent>>::size_type event_count =
        src_events.size();
    dest_events->resize(event_count);

    for (vector<linked_ptr<CommittedEvent>>::size_type i = 0; i < event_count;
         ++i) {
      (*dest_events)[i].reset(src_events[i]->Clone());
    }

    transaction_info->origin_peer = transaction.origin_peer();

    CHECK(transactions->emplace(transaction_id,
                                make_linked_ptr(transaction_info)).second);
  }

  ComputeEffectiveVersion_Locked(transaction_store_version_map,
                                 effective_version);
}

void VersionedObjectContent::StoreTransactions(
    const CanonicalPeer* origin_peer,
    map<TransactionId, linked_ptr<SharedObjectTransactionInfo>>* transactions,
    const MaxVersionMap& version_map) {
  CHECK(origin_peer != nullptr);
  CHECK(transactions != nullptr);

  MutexLock lock(&committed_versions_mu_);

  for (const auto& transaction_pair : *transactions) {
    const TransactionId& transaction_id = transaction_pair.first;
    SharedObjectTransactionInfo* const transaction_info =
        transaction_pair.second.get();

    CHECK(IsValidTransactionId(transaction_id));
    CHECK(transaction_info != nullptr);

    vector<linked_ptr<CommittedEvent>>* const events =
        &transaction_info->events;
    // TODO(dss): Rename this local variable to distinguish it from the
    // parameter 'origin_peer'.
    const CanonicalPeer* const origin_peer = transaction_info->origin_peer;

    CHECK(origin_peer != nullptr);

    linked_ptr<SharedObjectTransaction>& transaction =
        committed_versions_[transaction_id];

    if (transaction.get() == nullptr) {
      transaction.reset(new SharedObjectTransaction(events, origin_peer));
    }

    // TODO(dss): [BUG] The following statement should use the parameter named
    // 'origin_peer', not the local variable with the same name.
    version_map_.AddPeerTransactionId(origin_peer, transaction_id);
  }

  MaxVersionMap new_version_map;
  GetVersionMapUnion(version_map_, version_map, &new_version_map);
  version_map_.Swap(&new_version_map);

  up_to_date_peers_.insert(origin_peer);
}

void VersionedObjectContent::InsertTransaction(
    const CanonicalPeer* origin_peer, const TransactionId& transaction_id,
    vector<linked_ptr<CommittedEvent>>* events) {
  CHECK(origin_peer != nullptr);
  CHECK(IsValidTransactionId(transaction_id));
  CHECK(events != nullptr);

  MutexLock lock(&committed_versions_mu_);

  linked_ptr<SharedObjectTransaction>& transaction =
      committed_versions_[transaction_id];

  if (transaction.get() == nullptr) {
    transaction.reset(new SharedObjectTransaction(events, origin_peer));
  }

  version_map_.AddPeerTransactionId(origin_peer, transaction_id);
  up_to_date_peers_.insert(origin_peer);
}

void VersionedObjectContent::SetCachedLiveObject(
    const shared_ptr<const LiveObject>& cached_live_object,
    const SequencePointImpl& cached_sequence_point) {
  CHECK(cached_live_object.get() != nullptr);

  MutexLock lock(&committed_versions_mu_);
  cached_live_object_ = cached_live_object;
  cached_sequence_point_.CopyFrom(cached_sequence_point);
}

string VersionedObjectContent::Dump() const {
  MutexLock lock(&committed_versions_mu_);

  string committed_versions_string;
  if (committed_versions_.empty()) {
    committed_versions_string = "{}";
  } else {
    committed_versions_string = "{";

    for (map<TransactionId, linked_ptr<SharedObjectTransaction>>::const_iterator
             it = committed_versions_.begin();
         it != committed_versions_.end(); ++it) {
      if (it != committed_versions_.begin()) {
        committed_versions_string += ",";
      }

      StringAppendF(&committed_versions_string, " \"%s\": %s",
                    TransactionIdToString(it->first).c_str(),
                    it->second->Dump().c_str());
    }

    committed_versions_string += " }";
  }

  string up_to_date_peers_string;
  if (up_to_date_peers_.empty()) {
    up_to_date_peers_string = "[]";
  } else {
    up_to_date_peers_string = "[";

    for (unordered_set<const CanonicalPeer*>::const_iterator it =
             up_to_date_peers_.begin();
         it != up_to_date_peers_.end(); ++it) {
      if (it != up_to_date_peers_.begin()) {
        up_to_date_peers_string += ",";
      }

      StringAppendF(&up_to_date_peers_string, " \"%s\"",
                    CEscape((*it)->peer_id()).c_str());
    }

    up_to_date_peers_string += " ]";
  }

  return StringPrintf(
      "{ \"committed_versions\": %s, \"version_map\": %s, "
      "\"up_to_date_peers\": %s, \"cached_live_object\": %s, "
      "\"cached_sequence_point\": %s }",
      committed_versions_string.c_str(), version_map_.Dump().c_str(),
      up_to_date_peers_string.c_str(), cached_live_object_->Dump().c_str(),
      cached_sequence_point_.Dump().c_str());
}

bool VersionedObjectContent::ApplyTransactionsToWorkingVersion_Locked(
    PeerThread* peer_thread, const SequencePointImpl& sequence_point,
    vector<pair<const CanonicalPeer*, TransactionId>>* transactions_to_reject) {
  CHECK(peer_thread != nullptr);
  CHECK(transactions_to_reject != nullptr);

  for (const auto& transaction_pair : committed_versions_) {
    const TransactionId& transaction_id = transaction_pair.first;
    const SharedObjectTransaction& transaction = *transaction_pair.second;
    const vector<linked_ptr<CommittedEvent>>& events = transaction.events();

    if (!events.empty()) {
      const CanonicalPeer* const origin_peer = transaction.origin_peer();

      if (sequence_point.HasPeerTransactionId(origin_peer, transaction_id) &&
          FindTransactionIdInVector(*transactions_to_reject, transaction_id) ==
              transactions_to_reject->end()) {
        for (const linked_ptr<CommittedEvent>& event : events) {
          peer_thread->QueueEvent(event.get());
        }

        peer_thread->FlushEvents();

        if (peer_thread->conflict_detected()) {
          transactions_to_reject->emplace_back(origin_peer, transaction_id);
          return false;
        }
      }
    }
  }

  return true;
}

void VersionedObjectContent::ComputeEffectiveVersion_Locked(
    const MaxVersionMap& transaction_store_version_map,
    MaxVersionMap* effective_version) const {
  CHECK(effective_version != nullptr);

  const unordered_map<const CanonicalPeer*, TransactionId>&
      version_map_peer_transaction_ids = version_map_.peer_transaction_ids();

  for (const auto& version_map_pair : version_map_peer_transaction_ids) {
    effective_version->AddPeerTransactionId(version_map_pair.first,
                                            version_map_pair.second);
  }

  const unordered_map<const CanonicalPeer*, TransactionId>&
      transaction_store_peer_transaction_ids =
      transaction_store_version_map.peer_transaction_ids();

  for (const CanonicalPeer* const origin_peer : up_to_date_peers_) {
    const unordered_map<const CanonicalPeer*, TransactionId>::const_iterator
        it2 = transaction_store_peer_transaction_ids.find(origin_peer);

    if (it2 != transaction_store_peer_transaction_ids.end()) {
      effective_version->AddPeerTransactionId(origin_peer, it2->second);
    }
  }
}

bool VersionedObjectContent::CanUseCachedLiveObject_Locked(
    const SequencePointImpl& requested_sequence_point) const {
  if (cached_live_object_.get() == nullptr) {
    return false;
  }

  const MaxVersionMap& requested_version_map =
      requested_sequence_point.version_map();
  const MaxVersionMap& cached_version_map =
      cached_sequence_point_.version_map();

  if (!VersionMapIsLessThanOrEqual(cached_version_map, requested_version_map)) {
    return false;
  }

  const unordered_map<const CanonicalPeer*, TransactionId>&
      requested_peer_transactions_ids =
      requested_version_map.peer_transaction_ids();
  const unordered_map<const CanonicalPeer*, TransactionId>&
      cached_peer_transactions_ids = cached_version_map.peer_transaction_ids();

  for (const auto& requested_peer_pair : requested_peer_transactions_ids) {
    const CanonicalPeer* const origin_peer = requested_peer_pair.first;
    const TransactionId& requested_transaction_id = requested_peer_pair.second;

    const unordered_map<const CanonicalPeer*, TransactionId>::const_iterator
        cached_peer_it = cached_peer_transactions_ids.find(origin_peer);

    TransactionId cached_transaction_id;
    if (cached_peer_it == cached_peer_transactions_ids.end()) {
      GetMinTransactionId(&cached_transaction_id);
    } else {
      cached_transaction_id.CopyFrom(cached_peer_it->second);
    }

    const map<TransactionId, linked_ptr<SharedObjectTransaction>>::
        const_iterator start_it = committed_versions_.upper_bound(
        cached_transaction_id);
    const map<TransactionId, linked_ptr<SharedObjectTransaction>>::
        const_iterator end_it = committed_versions_.upper_bound(
        requested_transaction_id);

    for (map<TransactionId, linked_ptr<SharedObjectTransaction>>::const_iterator
             transaction_it = start_it;
         transaction_it != end_it; ++transaction_it) {
      const SharedObjectTransaction& shared_object_transaction =
          *transaction_it->second;

      if (shared_object_transaction.origin_peer() == origin_peer) {
        const vector<linked_ptr<CommittedEvent>>& events =
            shared_object_transaction.events();

        for (const linked_ptr<CommittedEvent>& event : events) {
          const CommittedEvent::Type event_type = event->type();

          if (event_type != CommittedEvent::METHOD_CALL &&
              event_type != CommittedEvent::SUB_METHOD_RETURN) {
            return false;
          }
        }
      }
    }
  }

  if (!PeerExclusionMapsAreEqual(requested_sequence_point.peer_exclusion_map(),
                                 cached_sequence_point_.peer_exclusion_map())) {
    return false;
  }

  if (requested_sequence_point.rejected_peers() !=
      cached_sequence_point_.rejected_peers()) {
    return false;
  }

  return true;
}

}  // namespace peer
}  // namespace floating_temple
