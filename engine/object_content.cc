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

#include "engine/object_content.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/escape.h"
#include "base/logging.h"
#include "base/mutex.h"
#include "base/mutex_lock.h"
#include "engine/canonical_peer.h"
#include "engine/committed_event.h"
#include "engine/live_object.h"
#include "engine/max_version_map.h"
#include "engine/peer_exclusion_map.h"
#include "engine/playback_thread.h"
#include "engine/proto/transaction_id.pb.h"
#include "engine/sequence_point_impl.h"
#include "engine/shared_object_transaction.h"
#include "engine/transaction_id_util.h"
#include "engine/transaction_store_internal_interface.h"
#include "engine/version_map.h"
#include "util/dump_context.h"
#include "util/dump_context_impl.h"

using std::map;
using std::pair;
using std::shared_ptr;
using std::unique_ptr;
using std::unordered_map;
using std::unordered_set;
using std::vector;

namespace floating_temple {
namespace engine {
namespace {

vector<pair<const CanonicalPeer*, TransactionId>>::const_iterator
FindTransactionIdInVector(
    const vector<pair<const CanonicalPeer*, TransactionId>>& transaction_pairs,
    const TransactionId& transaction_id) {
  const auto end_it = transaction_pairs.end();
  auto it = transaction_pairs.begin();

  while (it != end_it && it->second != transaction_id) {
    ++it;
  }

  return it;
}

}  // namespace

ObjectContent::ObjectContent(
    TransactionStoreInternalInterface* transaction_store,
    SharedObject* shared_object)
    : transaction_store_(CHECK_NOTNULL(transaction_store)),
      shared_object_(CHECK_NOTNULL(shared_object)),
      max_requested_transaction_id_(MIN_TRANSACTION_ID) {
}

ObjectContent::~ObjectContent() {
}

shared_ptr<const LiveObject> ObjectContent::GetWorkingVersion(
    const MaxVersionMap& transaction_store_version_map,
    const SequencePointImpl& sequence_point,
    unordered_map<SharedObject*, ObjectReferenceImpl*>* new_object_references,
    vector<pair<const CanonicalPeer*, TransactionId>>* transactions_to_reject) {
  MutexLock lock(&committed_versions_mu_);

  MaxVersionMap effective_version;
  ComputeEffectiveVersion_Locked(transaction_store_version_map,
                                 &effective_version);

  if (!VersionMapIsLessThanOrEqual(sequence_point.version_map(),
                                   effective_version)) {
    VLOG(1) << "sequence_point.version_map() == "
            << GetJsonString(sequence_point.version_map());
    VLOG(1) << "effective_version == " << GetJsonString(effective_version);

    return shared_ptr<const LiveObject>(nullptr);
  }

  if (CanUseCachedLiveObject_Locked(sequence_point)) {
    CHECK(cached_live_object_.get() != nullptr);
    return cached_live_object_;
  }

  return GetWorkingVersion_Locked(sequence_point.version_map(),
                                  new_object_references,
                                  transactions_to_reject);
}

void ObjectContent::GetTransactions(
    const MaxVersionMap& transaction_store_version_map,
    map<TransactionId, unique_ptr<SharedObjectTransaction>>* transactions,
    MaxVersionMap* effective_version) const {
  CHECK(transactions != nullptr);

  MutexLock lock(&committed_versions_mu_);

  for (const auto& transaction_pair : committed_versions_) {
    const TransactionId& transaction_id = transaction_pair.first;
    const SharedObjectTransaction* const transaction =
        transaction_pair.second.get();

    CHECK(transactions->emplace(
        transaction_id,
        unique_ptr<SharedObjectTransaction>(transaction->Clone())).second);
  }

  ComputeEffectiveVersion_Locked(transaction_store_version_map,
                                 effective_version);
}

void ObjectContent::StoreTransactions(
    const CanonicalPeer* remote_peer,
    const map<TransactionId, unique_ptr<SharedObjectTransaction>>& transactions,
    const MaxVersionMap& version_map,
    unordered_map<SharedObject*, ObjectReferenceImpl*>* new_object_references,
    vector<pair<const CanonicalPeer*, TransactionId>>* transactions_to_reject) {
  CHECK(remote_peer != nullptr);

  bool should_replay_transactions = false;

  MutexLock lock(&committed_versions_mu_);

  for (const auto& transaction_pair : transactions) {
    const TransactionId& transaction_id = transaction_pair.first;
    const unique_ptr<SharedObjectTransaction>& src_transaction =
        transaction_pair.second;

    CHECK(IsValidTransactionId(transaction_id));

    unique_ptr<SharedObjectTransaction>& dest_transaction =
        committed_versions_[transaction_id];
    if (dest_transaction.get() == nullptr) {
      dest_transaction.reset(src_transaction->Clone());

      if (transaction_id <= max_requested_transaction_id_) {
        should_replay_transactions = true;
      }
    }

    version_map_.AddPeerTransactionId(src_transaction->origin_peer(),
                                      transaction_id);
  }

  MaxVersionMap new_version_map;
  GetVersionMapUnion(version_map_, version_map, &new_version_map);
  version_map_.Swap(&new_version_map);

  up_to_date_peers_.insert(remote_peer);

  VLOG(1) << "should_replay_transactions == "
          << (should_replay_transactions ? "true" : "false");

  if (should_replay_transactions) {
    // TODO(dss): Use the 'new_object_references' parameter here instead of
    // creating a temporary map?
    unordered_map<SharedObject*, ObjectReferenceImpl*>
        new_object_references_temp;
    GetWorkingVersion_Locked(version_map_, &new_object_references_temp,
                             transactions_to_reject);
  }
}

void ObjectContent::InsertTransaction(
    const CanonicalPeer* origin_peer,
    const TransactionId& transaction_id,
    const vector<unique_ptr<CommittedEvent>>& events,
    bool transaction_is_local,
    unordered_map<SharedObject*, ObjectReferenceImpl*>* new_object_references,
    vector<pair<const CanonicalPeer*, TransactionId>>* transactions_to_reject) {
  CHECK(origin_peer != nullptr);
  CHECK(IsValidTransactionId(transaction_id));

  MutexLock lock(&committed_versions_mu_);

  unique_ptr<SharedObjectTransaction>& transaction =
      committed_versions_[transaction_id];

  if (transaction.get() == nullptr) {
    transaction.reset(new SharedObjectTransaction(events, origin_peer));
  }

  version_map_.AddPeerTransactionId(origin_peer, transaction_id);
  up_to_date_peers_.insert(origin_peer);

  if (transaction_id <= max_requested_transaction_id_) {
    // TODO(dss): Use the 'new_object_references' parameter here instead of
    // creating a temporary map?
    unordered_map<SharedObject*, ObjectReferenceImpl*>
        new_object_references_temp;
    GetWorkingVersion_Locked(version_map_, &new_object_references_temp,
                             transactions_to_reject);
  } else {
    if (transaction_is_local) {
      max_requested_transaction_id_ = transaction_id;
      VLOG(1) << "max_requested_transaction_id_ is now "
              << TransactionIdToString(max_requested_transaction_id_);
    }
  }
}

void ObjectContent::SetCachedLiveObject(
    const shared_ptr<const LiveObject>& cached_live_object,
    const SequencePointImpl& cached_sequence_point) {
  CHECK(cached_live_object.get() != nullptr);

  MutexLock lock(&committed_versions_mu_);
  cached_live_object_ = cached_live_object;
  cached_sequence_point_.CopyFrom(cached_sequence_point);
}

void ObjectContent::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  MutexLock lock(&committed_versions_mu_);

  dc->BeginMap();

  dc->AddString("committed_versions");
  dc->BeginMap();
  for (const auto& transaction_pair : committed_versions_) {
    dc->AddString(TransactionIdToString(transaction_pair.first));
    transaction_pair.second->Dump(dc);
  }
  dc->End();

  dc->AddString("version_map");
  version_map_.Dump(dc);

  dc->AddString("up_to_date_peers");
  dc->BeginList();
  for (const CanonicalPeer* const canonical_peer : up_to_date_peers_) {
    dc->AddString(canonical_peer->peer_id());
  }
  dc->End();

  dc->AddString("cached_live_object");
  if (cached_live_object_.get() == nullptr) {
    dc->AddNull();
  } else {
    cached_live_object_->Dump(dc);
  }

  dc->AddString("cached_sequence_point");
  cached_sequence_point_.Dump(dc);

  dc->End();
}

shared_ptr<const LiveObject> ObjectContent::GetWorkingVersion_Locked(
    const MaxVersionMap& desired_version,
    unordered_map<SharedObject*, ObjectReferenceImpl*>* new_object_references,
    vector<pair<const CanonicalPeer*, TransactionId>>* transactions_to_reject) {
  for (;;) {
    PlaybackThread playback_thread;
    playback_thread.Start(transaction_store_, shared_object_,
                          shared_ptr<LiveObject>(nullptr),
                          new_object_references);

    const bool success = ApplyTransactionsToWorkingVersion_Locked(
        &playback_thread, desired_version, transactions_to_reject);

    playback_thread.Stop();

    if (success) {
      return playback_thread.live_object();
    }
  }
}

bool ObjectContent::ApplyTransactionsToWorkingVersion_Locked(
    PlaybackThread* playback_thread, const MaxVersionMap& desired_version,
    vector<pair<const CanonicalPeer*, TransactionId>>* transactions_to_reject) {
  CHECK(playback_thread != nullptr);
  CHECK(transactions_to_reject != nullptr);

  for (const auto& transaction_pair : committed_versions_) {
    const TransactionId& transaction_id = transaction_pair.first;
    const SharedObjectTransaction& transaction = *transaction_pair.second;
    const vector<unique_ptr<CommittedEvent>>& events = transaction.events();

    if (!events.empty()) {
      const CanonicalPeer* const origin_peer = transaction.origin_peer();

      if (desired_version.HasPeerTransactionId(origin_peer, transaction_id) &&
          FindTransactionIdInVector(*transactions_to_reject, transaction_id) ==
              transactions_to_reject->end()) {
        for (const unique_ptr<CommittedEvent>& event : events) {
          playback_thread->QueueEvent(event.get());
        }

        playback_thread->FlushEvents();

        if (playback_thread->conflict_detected()) {
          transactions_to_reject->emplace_back(origin_peer, transaction_id);
          return false;
        }
      }
    }
  }

  return true;
}

void ObjectContent::ComputeEffectiveVersion_Locked(
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

bool ObjectContent::CanUseCachedLiveObject_Locked(
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
      cached_transaction_id = MIN_TRANSACTION_ID;
    } else {
      cached_transaction_id = cached_peer_it->second;
    }

    const map<TransactionId, unique_ptr<SharedObjectTransaction>>::
        const_iterator start_it = committed_versions_.upper_bound(
        cached_transaction_id);
    const map<TransactionId, unique_ptr<SharedObjectTransaction>>::
        const_iterator end_it = committed_versions_.upper_bound(
        requested_transaction_id);

    for (map<TransactionId, unique_ptr<SharedObjectTransaction>>::const_iterator
             transaction_it = start_it;
         transaction_it != end_it; ++transaction_it) {
      const SharedObjectTransaction& shared_object_transaction =
          *transaction_it->second;

      if (shared_object_transaction.origin_peer() == origin_peer) {
        const vector<unique_ptr<CommittedEvent>>& events =
            shared_object_transaction.events();

        for (const unique_ptr<CommittedEvent>& event : events) {
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

}  // namespace engine
}  // namespace floating_temple
