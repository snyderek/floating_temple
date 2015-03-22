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

#include "peer/shared_object.h"

#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <tr1/unordered_map>
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
#include "peer/const_live_object_ptr.h"
#include "peer/live_object.h"
#include "peer/live_object_ptr.h"
#include "peer/max_version_map.h"
#include "peer/peer_exclusion_map.h"
#include "peer/peer_object_impl.h"
#include "peer/peer_thread.h"
#include "peer/proto/transaction_id.pb.h"
#include "peer/proto/uuid.pb.h"
#include "peer/sequence_point_impl.h"
#include "peer/shared_object_transaction.h"
#include "peer/shared_object_transaction_info.h"
#include "peer/transaction_id_util.h"
#include "peer/transaction_store_internal_interface.h"
#include "peer/uuid_util.h"

using std::make_pair;
using std::map;
using std::pair;
using std::string;
using std::tr1::unordered_map;
using std::unordered_set;
using std::vector;

namespace floating_temple {
namespace peer {
namespace {

vector<pair<const CanonicalPeer*, TransactionId> >::const_iterator
FindTransactionIdInVector(
    const vector<pair<const CanonicalPeer*, TransactionId> >& transaction_pairs,
    const TransactionId& transaction_id) {
  const vector<pair<const CanonicalPeer*, TransactionId> >::const_iterator
      end_it = transaction_pairs.end();
  vector<pair<const CanonicalPeer*, TransactionId> >::const_iterator it =
      transaction_pairs.begin();

  while (it != end_it &&
         CompareTransactionIds(it->second, transaction_id) != 0) {
    ++it;
  }

  return it;
}

}  // namespace

SharedObject::SharedObject(TransactionStoreInternalInterface* transaction_store,
                           const Uuid& object_id)
    : transaction_store_(CHECK_NOTNULL(transaction_store)),
      object_id_(object_id) {
}

SharedObject::~SharedObject() {
}

void SharedObject::GetInterestedPeers(
    unordered_set<const CanonicalPeer*>* interested_peers) const {
  CHECK(interested_peers != NULL);

  MutexLock lock(&interested_peers_mu_);
  *interested_peers = interested_peers_;
}

void SharedObject::AddInterestedPeer(const CanonicalPeer* interested_peer) {
  CHECK(interested_peer != NULL);

  MutexLock lock(&interested_peers_mu_);
  interested_peers_.insert(interested_peer);
}

bool SharedObject::HasPeerObject(const PeerObjectImpl* peer_object) const {
  CHECK(peer_object != NULL);

  MutexLock lock(&peer_objects_mu_);

  for (const PeerObjectImpl* const matching_peer_object : peer_objects_) {
    if (matching_peer_object == peer_object) {
      return true;
    }
  }

  return false;
}

void SharedObject::AddPeerObject(PeerObjectImpl* peer_object) {
  CHECK(peer_object != NULL);

  MutexLock lock(&peer_objects_mu_);
  peer_objects_.push_back(peer_object);
}

PeerObjectImpl* SharedObject::GetOrCreatePeerObject() {
  {
    MutexLock lock(&peer_objects_mu_);

    if (!peer_objects_.empty()) {
      return peer_objects_.back();
    }
  }

  PeerObjectImpl* const new_peer_object =
      transaction_store_->CreateUnboundPeerObject();
  CHECK_EQ(new_peer_object->SetSharedObjectIfUnset(this), this);

  {
    MutexLock lock(&peer_objects_mu_);

    if (peer_objects_.empty()) {
      peer_objects_.push_back(new_peer_object);
      return new_peer_object;
    } else {
      // TODO(dss): Notify the transaction store that it can delete
      // new_peer_object.
      return peer_objects_.back();
    }
  }
}

ConstLiveObjectPtr SharedObject::GetWorkingVersion(
    const MaxVersionMap& transaction_store_version_map,
    const SequencePointImpl& sequence_point,
    unordered_map<SharedObject*, PeerObjectImpl*>* new_peer_objects,
    vector<pair<const CanonicalPeer*, TransactionId> >*
        transactions_to_reject) {
  MutexLock lock(&committed_versions_mu_);

  MaxVersionMap effective_version;
  ComputeEffectiveVersion_Locked(transaction_store_version_map,
                                 &effective_version);

  if (!VersionMapIsLessThanOrEqual(sequence_point.version_map(),
                                   effective_version)) {
    VLOG(1) << "sequence_point.version_map() == "
            << sequence_point.version_map().Dump();
    VLOG(1) << "effective_version == " << effective_version.Dump();

    return ConstLiveObjectPtr(NULL);
  }

  if (CanUseCachedLiveObject_Locked(sequence_point)) {
    CHECK(cached_live_object_.get() != NULL);
    return cached_live_object_;
  }

  for (;;) {
    PeerThread peer_thread;
    peer_thread.Start(transaction_store_, this, LiveObjectPtr(NULL),
                      new_peer_objects);

    const bool success = ApplyTransactionsToWorkingVersion_Locked(
        &peer_thread, sequence_point, transactions_to_reject);

    peer_thread.Stop();

    if (success) {
      return peer_thread.live_object();
    }
  }

  return ConstLiveObjectPtr(NULL);
}

void SharedObject::GetTransactions(
    const MaxVersionMap& transaction_store_version_map,
    map<TransactionId, linked_ptr<SharedObjectTransactionInfo> >* transactions,
    MaxVersionMap* effective_version) const {
  CHECK(transactions != NULL);

  MutexLock lock(&committed_versions_mu_);

  for (const auto& transaction_pair : committed_versions_) {
    const TransactionId& transaction_id = transaction_pair.first;
    const SharedObjectTransaction& transaction = *transaction_pair.second;

    SharedObjectTransactionInfo* const transaction_info =
        new SharedObjectTransactionInfo();

    const vector<linked_ptr<CommittedEvent> >& src_events =
        transaction.events();
    vector<linked_ptr<CommittedEvent> >* const dest_events =
        &transaction_info->events;
    const vector<linked_ptr<CommittedEvent> >::size_type event_count =
        src_events.size();
    dest_events->resize(event_count);

    for (vector<linked_ptr<CommittedEvent> >::size_type i = 0; i < event_count;
         ++i) {
      (*dest_events)[i].reset(src_events[i]->Clone());
    }

    transaction_info->origin_peer = transaction.origin_peer();

    CHECK(transactions->insert(
              make_pair(transaction_id,
                        make_linked_ptr(transaction_info))).second);
  }

  ComputeEffectiveVersion_Locked(transaction_store_version_map,
                                 effective_version);
}

void SharedObject::StoreTransactions(
    const CanonicalPeer* origin_peer,
    map<TransactionId, linked_ptr<SharedObjectTransactionInfo> >* transactions,
    const MaxVersionMap& version_map) {
  CHECK(origin_peer != NULL);
  CHECK(transactions != NULL);

  MutexLock lock(&committed_versions_mu_);

  for (const auto& transaction_pair : *transactions) {
    const TransactionId& transaction_id = transaction_pair.first;
    SharedObjectTransactionInfo* const transaction_info =
        transaction_pair.second.get();

    CHECK(IsValidTransactionId(transaction_id));
    CHECK(transaction_info != NULL);

    vector<linked_ptr<CommittedEvent> >* const events =
        &transaction_info->events;
    const CanonicalPeer* const origin_peer = transaction_info->origin_peer;

    CHECK(origin_peer != NULL);

    linked_ptr<SharedObjectTransaction>& transaction =
        committed_versions_[transaction_id];

    if (transaction.get() == NULL) {
      transaction.reset(new SharedObjectTransaction(events, origin_peer));
    }

    version_map_.AddPeerTransactionId(origin_peer, transaction_id);
  }

  MaxVersionMap new_version_map;
  GetVersionMapUnion(version_map_, version_map, &new_version_map);
  version_map_.Swap(&new_version_map);

  up_to_date_peers_.insert(origin_peer);
}

void SharedObject::InsertTransaction(
    const CanonicalPeer* origin_peer, const TransactionId& transaction_id,
    vector<linked_ptr<CommittedEvent> >* events) {
  CHECK(origin_peer != NULL);
  CHECK(IsValidTransactionId(transaction_id));
  CHECK(events != NULL);

  const vector<linked_ptr<CommittedEvent> >::size_type event_count =
      events->size();

  if (VLOG_IS_ON(2)) {
    for (vector<linked_ptr<CommittedEvent> >::size_type i = 0; i < event_count;
         ++i) {
      VLOG(2) << "Event " << i << ": " << (*events)[i]->Dump();
    }
  }

  MutexLock lock(&committed_versions_mu_);

  linked_ptr<SharedObjectTransaction>& transaction =
      committed_versions_[transaction_id];

  if (transaction.get() == NULL) {
    transaction.reset(new SharedObjectTransaction(events, origin_peer));
  }

  version_map_.AddPeerTransactionId(origin_peer, transaction_id);
  up_to_date_peers_.insert(origin_peer);
}

void SharedObject::SetCachedLiveObject(
    const ConstLiveObjectPtr& cached_live_object,
    const SequencePointImpl& cached_sequence_point) {
  CHECK(cached_live_object.get() != NULL);

  MutexLock lock(&committed_versions_mu_);
  cached_live_object_ = cached_live_object;
  cached_sequence_point_.CopyFrom(cached_sequence_point);
}

string SharedObject::Dump() const {
  MutexLock lock1(&committed_versions_mu_);
  MutexLock lock2(&interested_peers_mu_);
  MutexLock lock3(&peer_objects_mu_);

  return Dump_Locked();
}

bool SharedObject::ApplyTransactionsToWorkingVersion_Locked(
    PeerThread* peer_thread, const SequencePointImpl& sequence_point,
    vector<pair<const CanonicalPeer*, TransactionId> >*
        transactions_to_reject) {
  CHECK(peer_thread != NULL);
  CHECK(transactions_to_reject != NULL);

  for (const auto& transaction_pair : committed_versions_) {
    const TransactionId& transaction_id = transaction_pair.first;
    const SharedObjectTransaction& transaction = *transaction_pair.second;
    const vector<linked_ptr<CommittedEvent> >& events = transaction.events();

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
          transactions_to_reject->push_back(make_pair(origin_peer,
                                                      transaction_id));
          return false;
        }
      }
    }
  }

  return true;
}

void SharedObject::ComputeEffectiveVersion_Locked(
    const MaxVersionMap& transaction_store_version_map,
    MaxVersionMap* effective_version) const {
  CHECK(effective_version != NULL);

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

bool SharedObject::CanUseCachedLiveObject_Locked(
    const SequencePointImpl& requested_sequence_point) const {
  if (cached_live_object_.get() == NULL) {
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

    const map<TransactionId, linked_ptr<SharedObjectTransaction> >::
        const_iterator start_it = committed_versions_.upper_bound(
        cached_transaction_id);
    const map<TransactionId, linked_ptr<SharedObjectTransaction> >::
        const_iterator end_it = committed_versions_.upper_bound(
        requested_transaction_id);

    for (map<TransactionId, linked_ptr<SharedObjectTransaction> >::
             const_iterator transaction_it = start_it;
         transaction_it != end_it; ++transaction_it) {
      const SharedObjectTransaction& shared_object_transaction =
          *transaction_it->second;

      if (shared_object_transaction.origin_peer() == origin_peer) {
        const vector<linked_ptr<CommittedEvent> >& events =
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

string SharedObject::Dump_Locked() const {
  string interested_peer_ids_string;
  if (interested_peers_.empty()) {
    interested_peer_ids_string = "[]";
  } else {
    interested_peer_ids_string = "[";

    for (unordered_set<const CanonicalPeer*>::const_iterator it =
             interested_peers_.begin();
         it != interested_peers_.end(); ++it) {
      if (it != interested_peers_.begin()) {
        interested_peer_ids_string += ",";
      }

      StringAppendF(&interested_peer_ids_string, " \"%s\"",
                    CEscape((*it)->peer_id()).c_str());
    }

    interested_peer_ids_string += " ]";
  }

  string peer_objects_string;
  if (peer_objects_.empty()) {
    peer_objects_string = "[]";
  } else {
    peer_objects_string = "[";

    for (vector<PeerObjectImpl*>::const_iterator it = peer_objects_.begin();
         it != peer_objects_.end(); ++it) {
      if (it != peer_objects_.begin()) {
        peer_objects_string += ",";
      }

      StringAppendF(&peer_objects_string, " \"%p\"", *it);
    }

    peer_objects_string += " ]";
  }

  string committed_versions_string;
  if (committed_versions_.empty()) {
    committed_versions_string = "{}";
  } else {
    committed_versions_string = "{";

    for (map<TransactionId, linked_ptr<SharedObjectTransaction> >::
             const_iterator it = committed_versions_.begin();
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
      "{ \"object_id\": \"%s\", \"interested_peers\": %s, "
      "\"peer_objects\": %s, \"committed_versions\": %s, \"version_map\": %s, "
      "\"up_to_date_peers\": %s, \"cached_live_object\": %s, "
      "\"cached_sequence_point\": %s }",
      UuidToString(object_id_).c_str(), interested_peer_ids_string.c_str(),
      peer_objects_string.c_str(), committed_versions_string.c_str(),
      version_map_.Dump().c_str(), up_to_date_peers_string.c_str(),
      cached_live_object_->Dump().c_str(),
      cached_sequence_point_.Dump().c_str());
}

}  // namespace peer
}  // namespace floating_temple
