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

#ifndef ENGINE_VERSIONED_OBJECT_CONTENT_H_
#define ENGINE_VERSIONED_OBJECT_CONTENT_H_

#include <map>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/linked_ptr.h"
#include "base/macros.h"
#include "base/mutex.h"
#include "engine/max_version_map.h"
#include "engine/object_content.h"
#include "engine/proto/transaction_id.pb.h"
#include "engine/sequence_point_impl.h"
#include "engine/transaction_id_util.h"

namespace floating_temple {
namespace engine {

class CanonicalPeer;
class LiveObject;
class PlaybackThread;
class SharedObject;
class SharedObjectTransaction;
class TransactionStoreInternalInterface;
class Uuid;

class VersionedObjectContent : public ObjectContent {
 public:
  VersionedObjectContent(TransactionStoreInternalInterface* transaction_store,
                         SharedObject* shared_object);
  ~VersionedObjectContent() override;

  std::shared_ptr<const LiveObject> GetWorkingVersion(
      const MaxVersionMap& transaction_store_version_map,
      const SequencePointImpl& sequence_point,
      std::unordered_map<SharedObject*, ObjectReferenceImpl*>*
          new_object_references,
      std::vector<std::pair<const CanonicalPeer*, TransactionId>>*
          transactions_to_reject) override;
  void GetTransactions(
      const MaxVersionMap& transaction_store_version_map,
      std::map<TransactionId, linked_ptr<SharedObjectTransaction>>*
          transactions,
      MaxVersionMap* effective_version) const override;
  void StoreTransactions(
      const CanonicalPeer* remote_peer,
      const std::map<TransactionId, linked_ptr<SharedObjectTransaction>>&
          transactions,
      const MaxVersionMap& version_map,
      std::unordered_map<SharedObject*, ObjectReferenceImpl*>*
          new_object_references,
      std::vector<std::pair<const CanonicalPeer*, TransactionId>>*
          transactions_to_reject) override;
  void InsertTransaction(
      const CanonicalPeer* origin_peer,
      const TransactionId& transaction_id,
      const std::vector<linked_ptr<CommittedEvent>>& events,
      std::unordered_map<SharedObject*, ObjectReferenceImpl*>*
          new_object_references,
      std::vector<std::pair<const CanonicalPeer*, TransactionId>>*
          transactions_to_reject) override;
  void SetCachedLiveObject(
      const std::shared_ptr<const LiveObject>& cached_live_object,
      const SequencePointImpl& cached_sequence_point) override;
  std::string Dump() const override;

 private:
  std::shared_ptr<const LiveObject> GetWorkingVersionHelper(
      const MaxVersionMap& transaction_store_version_map,
      const SequencePointImpl& sequence_point,
      std::unordered_map<SharedObject*, ObjectReferenceImpl*>*
          new_object_references,
      std::vector<std::pair<const CanonicalPeer*, TransactionId>>*
          transactions_to_reject);
  std::shared_ptr<const LiveObject> GetWorkingVersion_Locked(
      const MaxVersionMap& desired_version,
      std::unordered_map<SharedObject*, ObjectReferenceImpl*>*
          new_object_references,
      std::vector<std::pair<const CanonicalPeer*, TransactionId>>*
          transactions_to_reject);
  bool ApplyTransactionsToWorkingVersion_Locked(
      PlaybackThread* playback_thread, const MaxVersionMap& desired_version,
      std::vector<std::pair<const CanonicalPeer*, TransactionId>>*
          transactions_to_reject);

  void ComputeEffectiveVersion_Locked(
      const MaxVersionMap& transaction_store_version_map,
      MaxVersionMap* effective_version) const;
  bool CanUseCachedLiveObject_Locked(
      const SequencePointImpl& requested_sequence_point) const;

  TransactionStoreInternalInterface* const transaction_store_;
  SharedObject* const shared_object_;

  std::map<TransactionId, linked_ptr<SharedObjectTransaction>>
      committed_versions_;
  MaxVersionMap version_map_;
  std::unordered_set<const CanonicalPeer*> up_to_date_peers_;
  TransactionId max_requested_transaction_id_;
  std::shared_ptr<const LiveObject> cached_live_object_;
  SequencePointImpl cached_sequence_point_;
  mutable Mutex committed_versions_mu_;

  DISALLOW_COPY_AND_ASSIGN(VersionedObjectContent);
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_VERSIONED_OBJECT_CONTENT_H_