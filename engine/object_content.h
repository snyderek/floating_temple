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

#ifndef ENGINE_OBJECT_CONTENT_H_
#define ENGINE_OBJECT_CONTENT_H_

#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/mutex.h"
#include "engine/max_version_map.h"
#include "engine/proto/transaction_id.pb.h"
#include "engine/sequence_point_impl.h"
#include "engine/transaction_id_util.h"

namespace floating_temple {

class DumpContext;

namespace engine {

class CanonicalPeer;
class CommittedEvent;
class LiveObject;
class ObjectReferenceImpl;
class PlaybackThread;
class SequencePointImpl;
class SharedObject;
class SharedObjectTransaction;
class TransactionStoreInternalInterface;
class Uuid;

class ObjectContent {
 public:
  ObjectContent(TransactionStoreInternalInterface* transaction_store,
                SharedObject* shared_object);
  ~ObjectContent();

  std::shared_ptr<const LiveObject> GetWorkingVersion(
      const MaxVersionMap& transaction_store_version_map,
      const SequencePointImpl& sequence_point,
      std::unordered_map<SharedObject*, ObjectReferenceImpl*>*
          new_object_references,
      std::vector<std::pair<const CanonicalPeer*, TransactionId>>*
          transactions_to_reject);

  void GetTransactions(
      const MaxVersionMap& transaction_store_version_map,
      std::map<TransactionId, std::unique_ptr<SharedObjectTransaction>>*
          transactions,
      MaxVersionMap* effective_version) const;
  void StoreTransactions(
      const CanonicalPeer* remote_peer,
      const std::map<TransactionId, std::unique_ptr<SharedObjectTransaction>>&
          transactions,
      const MaxVersionMap& version_map,
      std::unordered_map<SharedObject*, ObjectReferenceImpl*>*
          new_object_references,
      std::vector<std::pair<const CanonicalPeer*, TransactionId>>*
          transactions_to_reject);

  void InsertTransaction(
      const CanonicalPeer* origin_peer,
      const TransactionId& transaction_id,
      const std::vector<std::unique_ptr<CommittedEvent>>& events,
      bool transaction_is_local,
      std::unordered_map<SharedObject*, ObjectReferenceImpl*>*
          new_object_references,
      std::vector<std::pair<const CanonicalPeer*, TransactionId>>*
          transactions_to_reject);

  void SetCachedLiveObject(
      const std::shared_ptr<const LiveObject>& cached_live_object,
      const SequencePointImpl& cached_sequence_point);

  void Dump(DumpContext* dc) const;

 private:
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

  std::map<TransactionId, std::unique_ptr<SharedObjectTransaction>>
      committed_versions_;
  MaxVersionMap version_map_;
  std::unordered_set<const CanonicalPeer*> up_to_date_peers_;
  // TODO(dss): Rename this member variable. It's the max transaction ID
  // committed by a recording thread on the local peer.
  TransactionId max_requested_transaction_id_;
  std::shared_ptr<const LiveObject> cached_live_object_;
  SequencePointImpl cached_sequence_point_;
  mutable Mutex committed_versions_mu_;

  DISALLOW_COPY_AND_ASSIGN(ObjectContent);
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_OBJECT_CONTENT_H_
