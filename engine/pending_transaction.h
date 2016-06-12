// Floating Temple
// Copyright 2016 Derek S. Snyder
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

#ifndef ENGINE_PENDING_TRANSACTION_H_
#define ENGINE_PENDING_TRANSACTION_H_

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "base/macros.h"
#include "engine/proto/transaction_id.pb.h"

namespace floating_temple {
namespace engine {

class LiveObject;
class ObjectReferenceImpl;
class PendingEvent;
class SequencePoint;
class TransactionStoreInternalInterface;

class PendingTransaction {
 public:
  PendingTransaction(TransactionStoreInternalInterface* transaction_store,
                     const TransactionId& base_transaction_id);
  ~PendingTransaction();

  const TransactionId& base_transaction_id() const
      { return base_transaction_id_; }

  bool IsEmpty() const { return events_.empty(); }

  std::shared_ptr<LiveObject> GetLiveObject(
      ObjectReferenceImpl* object_reference);
  bool IsObjectKnown(ObjectReferenceImpl* object_reference);

  bool AddNewObject(ObjectReferenceImpl* object_reference,
                    const std::shared_ptr<const LiveObject>& live_object);
  void UpdateLiveObject(ObjectReferenceImpl* object_reference,
                        const std::shared_ptr<LiveObject>& live_object);

  void AddEvent(PendingEvent* event);

  void Commit(TransactionId* transaction_id,
              std::unordered_set<ObjectReferenceImpl*>* new_objects);

 private:
  const SequencePoint* GetSequencePoint();

  void LogDebugInfo() const;

  TransactionStoreInternalInterface* const transaction_store_;
  // ID of the committed transaction that this pending transaction is based on.
  const TransactionId base_transaction_id_;

  std::unique_ptr<SequencePoint> sequence_point_;

  std::vector<std::unique_ptr<PendingEvent>> events_;
  std::unordered_map<ObjectReferenceImpl*, std::shared_ptr<LiveObject>>
      modified_objects_;
  std::unordered_set<ObjectReferenceImpl*> new_objects_;

  DISALLOW_COPY_AND_ASSIGN(PendingTransaction);
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_PENDING_TRANSACTION_H_
