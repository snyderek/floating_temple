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
#include "peer/object_content.h"
#include "peer/peer_object_impl.h"
#include "peer/proto/transaction_id.pb.h"
#include "peer/proto/uuid.pb.h"
#include "peer/shared_object_transaction_info.h"
#include "peer/transaction_id_util.h"
#include "peer/transaction_store_internal_interface.h"
#include "peer/unversioned_object_content.h"
#include "peer/uuid_util.h"
#include "peer/versioned_object_content.h"

using std::map;
using std::pair;
using std::shared_ptr;
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

namespace floating_temple {
namespace peer {

SharedObject::SharedObject(TransactionStoreInternalInterface* transaction_store,
                           const Uuid& object_id)
    : transaction_store_(CHECK_NOTNULL(transaction_store)),
      object_id_(object_id) {
}

SharedObject::~SharedObject() {
}

void SharedObject::GetInterestedPeers(
    unordered_set<const CanonicalPeer*>* interested_peers) const {
  CHECK(interested_peers != nullptr);

  MutexLock lock(&interested_peers_mu_);
  *interested_peers = interested_peers_;
}

void SharedObject::AddInterestedPeer(const CanonicalPeer* interested_peer) {
  CHECK(interested_peer != nullptr);

  MutexLock lock(&interested_peers_mu_);
  interested_peers_.insert(interested_peer);
}

bool SharedObject::HasPeerObject(const PeerObjectImpl* peer_object) const {
  CHECK(peer_object != nullptr);

  MutexLock lock(&peer_objects_mu_);

  for (const PeerObjectImpl* const matching_peer_object : peer_objects_) {
    if (matching_peer_object == peer_object) {
      return true;
    }
  }

  return false;
}

void SharedObject::AddPeerObject(PeerObjectImpl* new_peer_object) {
  CHECK(new_peer_object != nullptr);

  const bool versioned = new_peer_object->versioned();

  MutexLock lock(&peer_objects_mu_);

  if (!peer_objects_.empty()) {
    CHECK_EQ(versioned, peer_objects_.front()->versioned());
  }

  peer_objects_.push_back(new_peer_object);
}

PeerObjectImpl* SharedObject::GetOrCreatePeerObject(bool versioned) {
  {
    MutexLock lock(&peer_objects_mu_);

    if (!peer_objects_.empty()) {
      return peer_objects_.back();
    }
  }

  PeerObjectImpl* const new_peer_object =
      transaction_store_->CreateUnboundPeerObject(versioned);
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

void SharedObject::CreateUnversionedObjectContent(
    const shared_ptr<LiveObject>& live_object) {
  {
    MutexLock lock(&peer_objects_mu_);

    if (!peer_objects_.empty()) {
      CHECK(!peer_objects_.front()->versioned());
    }
  }

  {
    MutexLock lock(&object_content_mu_);

    if (object_content_.get() == nullptr) {
      object_content_.reset(new UnversionedObjectContent(transaction_store_,
                                                         live_object));
    }
  }
}

shared_ptr<const LiveObject> SharedObject::GetWorkingVersion(
    const MaxVersionMap& transaction_store_version_map,
    const SequencePointImpl& sequence_point,
    unordered_map<SharedObject*, PeerObjectImpl*>* new_peer_objects,
    vector<pair<const CanonicalPeer*, TransactionId>>* transactions_to_reject) {
  ObjectContent* const object_content_temp = GetObjectContent();

  if (object_content_temp == nullptr) {
    return shared_ptr<const LiveObject>(nullptr);
  }

  return object_content_temp->GetWorkingVersion(transaction_store_version_map,
                                                sequence_point,
                                                new_peer_objects,
                                                transactions_to_reject);
}

void SharedObject::GetTransactions(
    const MaxVersionMap& transaction_store_version_map,
    map<TransactionId, linked_ptr<SharedObjectTransactionInfo>>* transactions,
    MaxVersionMap* effective_version) {
  ObjectContent* const object_content_temp = GetObjectContent();

  if (object_content_temp == nullptr) {
    return;
  }

  return object_content_temp->GetTransactions(transaction_store_version_map,
                                              transactions, effective_version);
}

void SharedObject::StoreTransactions(
    const CanonicalPeer* remote_peer,
    map<TransactionId, linked_ptr<SharedObjectTransactionInfo>>* transactions,
    const MaxVersionMap& version_map) {
  GetOrCreateObjectContent()->StoreTransactions(remote_peer, transactions,
                                                version_map);
}

void SharedObject::InsertTransaction(
    const CanonicalPeer* origin_peer, const TransactionId& transaction_id,
    vector<linked_ptr<CommittedEvent>>* events) {
  const vector<linked_ptr<CommittedEvent>>::size_type event_count =
      events->size();

  if (VLOG_IS_ON(2)) {
    for (vector<linked_ptr<CommittedEvent>>::size_type i = 0; i < event_count;
         ++i) {
      VLOG(2) << "Event " << i << ": " << (*events)[i]->Dump();
    }
  }

  GetOrCreateObjectContent()->InsertTransaction(origin_peer, transaction_id,
                                                events);
}

void SharedObject::SetCachedLiveObject(
    const shared_ptr<const LiveObject>& cached_live_object,
    const SequencePointImpl& cached_sequence_point) {
  ObjectContent* const object_content_temp = GetObjectContent();

  if (object_content_temp == nullptr) {
    return;
  }

  object_content_temp->SetCachedLiveObject(cached_live_object,
                                           cached_sequence_point);
}

string SharedObject::Dump() const {
  MutexLock lock1(&interested_peers_mu_);
  MutexLock lock2(&peer_objects_mu_);
  MutexLock lock3(&object_content_mu_);

  return Dump_Locked();
}

ObjectContent* SharedObject::GetObjectContent() {
  MutexLock lock(&object_content_mu_);
  return object_content_.get();
}

ObjectContent* SharedObject::GetOrCreateObjectContent() {
  MutexLock lock(&object_content_mu_);

  if (object_content_.get() == nullptr) {
    object_content_.reset(new VersionedObjectContent(transaction_store_, this));
  }

  return object_content_.get();
}

// TODO(dss): Inline this method into SharedObject::Dump.
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

  string versioned_object_string;
  if (object_content_ == nullptr) {
    versioned_object_string = "null";
  } else {
    versioned_object_string = object_content_->Dump();
  }

  return StringPrintf(
      "{ \"object_id\": \"%s\", \"interested_peers\": %s, "
      "\"peer_objects\": %s, \"versioned_object\": %s }",
      UuidToString(object_id_).c_str(), interested_peer_ids_string.c_str(),
      peer_objects_string.c_str(), versioned_object_string.c_str());
}

}  // namespace peer
}  // namespace floating_temple
