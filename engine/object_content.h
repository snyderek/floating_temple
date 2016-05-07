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
#include <utility>
#include <vector>

#include "engine/max_version_map.h"
#include "engine/proto/transaction_id.pb.h"
#include "engine/transaction_id_util.h"

namespace floating_temple {

class DumpContext;

namespace engine {

class CanonicalPeer;
class CommittedEvent;
class LiveObject;
class ObjectReferenceImpl;
class SequencePointImpl;
class SharedObject;
class SharedObjectTransaction;

class ObjectContent {
 public:
  virtual ~ObjectContent() {}

  virtual std::shared_ptr<const LiveObject> GetWorkingVersion(
      const MaxVersionMap& transaction_store_version_map,
      const SequencePointImpl& sequence_point,
      std::unordered_map<SharedObject*, ObjectReferenceImpl*>*
          new_object_references,
      std::vector<std::pair<const CanonicalPeer*, TransactionId>>*
          transactions_to_reject) = 0;

  virtual void GetTransactions(
      const MaxVersionMap& transaction_store_version_map,
      std::map<TransactionId, std::unique_ptr<SharedObjectTransaction>>*
          transactions,
      MaxVersionMap* effective_version) const = 0;
  virtual void StoreTransactions(
      const CanonicalPeer* remote_peer,
      const std::map<TransactionId, std::unique_ptr<SharedObjectTransaction>>&
          transactions,
      const MaxVersionMap& version_map,
      std::unordered_map<SharedObject*, ObjectReferenceImpl*>*
          new_object_references,
      std::vector<std::pair<const CanonicalPeer*, TransactionId>>*
          transactions_to_reject) = 0;

  virtual void InsertTransaction(
      const CanonicalPeer* origin_peer,
      const TransactionId& transaction_id,
      const std::vector<std::unique_ptr<CommittedEvent>>& events,
      bool transaction_is_local,
      std::unordered_map<SharedObject*, ObjectReferenceImpl*>*
          new_object_references,
      std::vector<std::pair<const CanonicalPeer*, TransactionId>>*
          transactions_to_reject) = 0;

  virtual void SetCachedLiveObject(
      const std::shared_ptr<const LiveObject>& cached_live_object,
      const SequencePointImpl& cached_sequence_point) = 0;

  virtual void Dump(DumpContext* dc) const = 0;
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_OBJECT_CONTENT_H_
