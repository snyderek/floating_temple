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

#ifndef PEER_SHARED_OBJECT_H_
#define PEER_SHARED_OBJECT_H_

#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/linked_ptr.h"
#include "base/macros.h"
#include "base/mutex.h"
#include "peer/const_live_object_ptr.h"
#include "peer/max_version_map.h"
#include "peer/proto/transaction_id.pb.h"
#include "peer/proto/uuid.pb.h"
#include "peer/sequence_point_impl.h"
#include "peer/transaction_id_util.h"

namespace floating_temple {
namespace peer {

class CanonicalPeer;
class CommittedEvent;
class PeerObjectImpl;
class PeerThread;
class SharedObjectTransaction;
class SharedObjectTransactionInfo;
class TransactionStoreInternalInterface;

class SharedObject {
 public:
  SharedObject(TransactionStoreInternalInterface* transaction_store,
               const Uuid& object_id);
  ~SharedObject();

  const Uuid& object_id() const { return object_id_; }

  void GetInterestedPeers(
      std::unordered_set<const CanonicalPeer*>* interested_peers) const;
  void AddInterestedPeer(const CanonicalPeer* interested_peer);

  bool HasPeerObject(const PeerObjectImpl* peer_object) const;
  void AddPeerObject(PeerObjectImpl* peer_object);
  PeerObjectImpl* GetOrCreatePeerObject();

  // TODO(dss): In the public methods below, 'new_peer_objects' is both an input
  // parameter and an output parameter. This is confusing. Try to come up with a
  // more intuitive API.

  ConstLiveObjectPtr GetWorkingVersion(
      const MaxVersionMap& transaction_store_version_map,
      const SequencePointImpl& sequence_point,
      std::unordered_map<SharedObject*, PeerObjectImpl*>* new_peer_objects,
      std::vector<std::pair<const CanonicalPeer*, TransactionId>>*
          transactions_to_reject);

  void GetTransactions(
      const MaxVersionMap& transaction_store_version_map,
      std::map<TransactionId, linked_ptr<SharedObjectTransactionInfo>>*
          transactions,
      MaxVersionMap* effective_version) const;
  void StoreTransactions(
      const CanonicalPeer* origin_peer,
      std::map<TransactionId, linked_ptr<SharedObjectTransactionInfo>>*
          transactions,
      const MaxVersionMap& version_map);

  void InsertTransaction(const CanonicalPeer* origin_peer,
                         const TransactionId& transaction_id,
                         std::vector<linked_ptr<CommittedEvent>>* events);

  void SetCachedLiveObject(const ConstLiveObjectPtr& cached_live_object,
                           const SequencePointImpl& cached_sequence_point);

  std::string Dump() const;

 private:
  bool ApplyTransactionsToWorkingVersion_Locked(
      PeerThread* peer_thread, const SequencePointImpl& sequence_point,
      std::vector<std::pair<const CanonicalPeer*, TransactionId>>*
          transactions_to_reject);

  void ComputeEffectiveVersion_Locked(
      const MaxVersionMap& transaction_store_version_map,
      MaxVersionMap* effective_version) const;
  bool CanUseCachedLiveObject_Locked(
      const SequencePointImpl& requested_sequence_point) const;

  std::string Dump_Locked() const;

  TransactionStoreInternalInterface* const transaction_store_;
  const Uuid object_id_;

  std::unordered_set<const CanonicalPeer*> interested_peers_;
  mutable Mutex interested_peers_mu_;

  std::vector<PeerObjectImpl*> peer_objects_;
  mutable Mutex peer_objects_mu_;

  std::map<TransactionId, linked_ptr<SharedObjectTransaction>>
      committed_versions_;
  MaxVersionMap version_map_;
  std::unordered_set<const CanonicalPeer*> up_to_date_peers_;
  ConstLiveObjectPtr cached_live_object_;
  SequencePointImpl cached_sequence_point_;
  mutable Mutex committed_versions_mu_;

  DISALLOW_COPY_AND_ASSIGN(SharedObject);
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_SHARED_OBJECT_H_
