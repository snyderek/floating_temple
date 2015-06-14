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

#ifndef PEER_UNVERSIONED_OBJECT_CONTENT_H_
#define PEER_UNVERSIONED_OBJECT_CONTENT_H_

#include <memory>

#include "base/macros.h"
#include "peer/object_content.h"

namespace floating_temple {

class UnversionedLocalObject;

namespace peer {

class LiveObject;
class TransactionStoreInternalInterface;

class UnversionedObjectContent : public ObjectContent {
 public:
  UnversionedObjectContent(
      TransactionStoreInternalInterface* transaction_store,
      const std::shared_ptr<const LiveObject>& live_object);
  ~UnversionedObjectContent() override;

  std::shared_ptr<const LiveObject> GetWorkingVersion(
      const MaxVersionMap& transaction_store_version_map,
      const SequencePointImpl& sequence_point,
      std::unordered_map<SharedObject*, PeerObjectImpl*>* new_peer_objects,
      std::vector<std::pair<const CanonicalPeer*, TransactionId>>*
          transactions_to_reject) override;
  void GetTransactions(
      const MaxVersionMap& transaction_store_version_map,
      std::map<TransactionId, linked_ptr<SharedObjectTransactionInfo>>*
          transactions,
      MaxVersionMap* effective_version) const override;
  void StoreTransactions(
      const CanonicalPeer* remote_peer,
      std::map<TransactionId, linked_ptr<SharedObjectTransactionInfo>>*
          transactions,
      const MaxVersionMap& version_map) override;
  void InsertTransaction(
      const CanonicalPeer* origin_peer, const TransactionId& transaction_id,
      std::vector<linked_ptr<CommittedEvent>>* events) override;
  void SetCachedLiveObject(
      const std::shared_ptr<const LiveObject>& cached_live_object,
      const SequencePointImpl& cached_sequence_point) override;
  std::string Dump() const override;

 private:
  TransactionStoreInternalInterface* const transaction_store_;
  const std::shared_ptr<const LiveObject> live_object_;

  DISALLOW_COPY_AND_ASSIGN(UnversionedObjectContent);
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_UNVERSIONED_OBJECT_CONTENT_H_
