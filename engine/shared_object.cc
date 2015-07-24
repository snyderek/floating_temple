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

#include "engine/shared_object.h"

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
#include "engine/canonical_peer.h"
#include "engine/committed_event.h"
#include "engine/live_object.h"
#include "engine/max_version_map.h"
#include "engine/object_content.h"
#include "engine/object_reference_impl.h"
#include "engine/proto/transaction_id.pb.h"
#include "engine/proto/uuid.pb.h"
#include "engine/shared_object_transaction.h"
#include "engine/transaction_id_util.h"
#include "engine/transaction_store_internal_interface.h"
#include "engine/unversioned_object_content.h"
#include "engine/uuid_util.h"
#include "engine/versioned_object_content.h"

using std::map;
using std::pair;
using std::shared_ptr;
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

namespace floating_temple {
namespace engine {

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

bool SharedObject::HasObjectReference(
    const ObjectReferenceImpl* object_reference) const {
  CHECK(object_reference != nullptr);

  MutexLock lock(&object_references_mu_);

  for (const ObjectReferenceImpl* const matching_object_reference :
           object_references_) {
    if (matching_object_reference == object_reference) {
      return true;
    }
  }

  return false;
}

void SharedObject::AddObjectReference(
    ObjectReferenceImpl* new_object_reference) {
  CHECK(new_object_reference != nullptr);

  const bool versioned = new_object_reference->versioned();

  MutexLock lock(&object_references_mu_);

  if (!object_references_.empty()) {
    CHECK_EQ(versioned, object_references_.front()->versioned());
  }

  object_references_.push_back(new_object_reference);
}

ObjectReferenceImpl* SharedObject::GetOrCreateObjectReference(bool versioned) {
  {
    MutexLock lock(&object_references_mu_);

    if (!object_references_.empty()) {
      return object_references_.back();
    }
  }

  ObjectReferenceImpl* const new_object_reference =
      transaction_store_->CreateUnboundObjectReference(versioned);
  CHECK_EQ(new_object_reference->SetSharedObjectIfUnset(this), this);

  {
    MutexLock lock(&object_references_mu_);

    if (object_references_.empty()) {
      object_references_.push_back(new_object_reference);
      return new_object_reference;
    } else {
      // TODO(dss): Notify the transaction store that it can delete
      // new_object_reference.
      return object_references_.back();
    }
  }
}

void SharedObject::CreateUnversionedObjectContent(
    const shared_ptr<LiveObject>& live_object) {
  {
    MutexLock lock(&object_references_mu_);

    if (!object_references_.empty()) {
      CHECK(!object_references_.front()->versioned());
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
    unordered_map<SharedObject*, ObjectReferenceImpl*>* new_object_references,
    vector<pair<const CanonicalPeer*, TransactionId>>* transactions_to_reject) {
  ObjectContent* const object_content_temp = GetObjectContent();

  if (object_content_temp == nullptr) {
    return shared_ptr<const LiveObject>(nullptr);
  }

  return object_content_temp->GetWorkingVersion(transaction_store_version_map,
                                                sequence_point,
                                                new_object_references,
                                                transactions_to_reject);
}

void SharedObject::GetTransactions(
    const MaxVersionMap& transaction_store_version_map,
    map<TransactionId, linked_ptr<SharedObjectTransaction>>* transactions,
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
    const map<TransactionId, linked_ptr<SharedObjectTransaction>>& transactions,
    const MaxVersionMap& version_map,
    unordered_map<SharedObject*, ObjectReferenceImpl*>* new_object_references,
    vector<pair<const CanonicalPeer*, TransactionId>>* transactions_to_reject) {
  GetOrCreateObjectContent()->StoreTransactions(remote_peer, transactions,
                                                version_map,
                                                new_object_references,
                                                transactions_to_reject);
}

void SharedObject::InsertTransaction(
    const CanonicalPeer* origin_peer,
    const TransactionId& transaction_id,
    const vector<linked_ptr<CommittedEvent>>& events,
    unordered_map<SharedObject*, ObjectReferenceImpl*>* new_object_references,
    vector<pair<const CanonicalPeer*, TransactionId>>* transactions_to_reject) {
  const vector<linked_ptr<CommittedEvent>>::size_type event_count =
      events.size();

  if (VLOG_IS_ON(2)) {
    for (vector<linked_ptr<CommittedEvent>>::size_type i = 0; i < event_count;
         ++i) {
      VLOG(2) << "Event " << i << ": " << events[i]->Dump();
    }
  }

  GetOrCreateObjectContent()->InsertTransaction(origin_peer, transaction_id,
                                                events, new_object_references,
                                                transactions_to_reject);
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
  MutexLock lock2(&object_references_mu_);
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

  string object_references_string;
  if (object_references_.empty()) {
    object_references_string = "[]";
  } else {
    object_references_string = "[";

    for (vector<ObjectReferenceImpl*>::const_iterator it =
             object_references_.begin();
         it != object_references_.end(); ++it) {
      if (it != object_references_.begin()) {
        object_references_string += ",";
      }

      StringAppendF(&object_references_string, " \"%p\"", *it);
    }

    object_references_string += " ]";
  }

  string versioned_object_string;
  if (object_content_ == nullptr) {
    versioned_object_string = "null";
  } else {
    versioned_object_string = object_content_->Dump();
  }

  return StringPrintf(
      "{ \"object_id\": \"%s\", \"interested_peers\": %s, "
      "\"object_references\": %s, \"versioned_object\": %s }",
      UuidToString(object_id_).c_str(), interested_peer_ids_string.c_str(),
      object_references_string.c_str(), versioned_object_string.c_str());
}

}  // namespace engine
}  // namespace floating_temple
