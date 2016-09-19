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

#ifndef ENGINE_SHARED_OBJECT_H_
#define ENGINE_SHARED_OBJECT_H_

#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/mutex.h"
#include "engine/live_object.h"
#include "engine/max_version_map.h"
#include "engine/proto/transaction_id.pb.h"
#include "engine/proto/uuid.pb.h"
#include "engine/transaction_id_util.h"

namespace floating_temple {

class DumpContext;

namespace engine {

class CanonicalPeer;
class CommittedEvent;
class ObjectContent;
class ObjectReferenceImpl;
class SequencePointImpl;
class SharedObjectTransaction;
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

  bool HasObjectReference(const ObjectReferenceImpl* object_reference) const;
  bool HasAnyObjectReference(
      const std::unordered_set<ObjectReferenceImpl*>& object_references) const;

  void AddObjectReference(ObjectReferenceImpl* new_object_reference);
  ObjectReferenceImpl* GetOrCreateObjectReference();

  // TODO(dss): In the public methods below, 'new_object_references' is both an
  // input parameter and an output parameter. This is confusing. Try to come up
  // with a more intuitive API.

  std::shared_ptr<const LiveObject> GetWorkingVersion(
      const MaxVersionMap& transaction_store_version_map,
      const SequencePointImpl& sequence_point,
      std::vector<std::pair<const CanonicalPeer*, TransactionId>>*
          transactions_to_reject);

  void GetTransactions(
      const MaxVersionMap& transaction_store_version_map,
      std::map<TransactionId, std::unique_ptr<SharedObjectTransaction>>*
          transactions,
      MaxVersionMap* effective_version);
  void StoreTransactions(
      const CanonicalPeer* remote_peer,
      const std::map<TransactionId, std::unique_ptr<SharedObjectTransaction>>&
          transactions,
      const MaxVersionMap& version_map,
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
  ObjectContent* GetObjectContent();
  // TODO(dss): Consider renaming this method to CreateObjectContent.
  ObjectContent* GetOrCreateObjectContent();

  void Dump_Locked(DumpContext* dc) const;

  TransactionStoreInternalInterface* const transaction_store_;
  const Uuid object_id_;

  std::unordered_set<const CanonicalPeer*> interested_peers_;
  mutable Mutex interested_peers_mu_;

  std::vector<ObjectReferenceImpl*> object_references_;
  mutable Mutex object_references_mu_;

  std::unique_ptr<ObjectContent> object_content_;
  mutable Mutex object_content_mu_;

  DISALLOW_COPY_AND_ASSIGN(SharedObject);
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_SHARED_OBJECT_H_
