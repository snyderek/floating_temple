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

#include "base/macros.h"
#include "engine/proto/transaction_id.pb.h"

namespace floating_temple {
namespace engine {

class CommittedEvent;
class LiveObject;
class ObjectReferenceImpl;
class SequencePoint;
class SharedObjectTransaction;
class TransactionStoreInternalInterface;

class PendingTransaction {
 public:
  // Does not take ownership of *transaction_store. Takes ownership of
  // *sequence_point.
  PendingTransaction(TransactionStoreInternalInterface* transaction_store,
                     const TransactionId& base_transaction_id,
                     SequencePoint* sequence_point);
  ~PendingTransaction();

  const TransactionId& base_transaction_id() const
      { return base_transaction_id_; }
  int transaction_level() const { return transaction_level_; }

  std::shared_ptr<LiveObject> GetLiveObject(
      ObjectReferenceImpl* object_reference);

  void AddNewObject(ObjectReferenceImpl* object_reference,
                    const std::shared_ptr<const LiveObject>& live_object,
                    bool object_is_named);
  void UpdateLiveObject(ObjectReferenceImpl* object_reference,
                        const std::shared_ptr<LiveObject>& live_object);

  void AddEvent(ObjectReferenceImpl* object_reference, CommittedEvent* event);

  void IncrementTransactionLevel();
  bool DecrementTransactionLevel();

  void Commit(TransactionId* transaction_id);

 private:
  void LogDebugInfo() const;

  TransactionStoreInternalInterface* const transaction_store_;
  // ID of the committed transaction that this pending transaction is based on.
  const TransactionId base_transaction_id_;
  const std::unique_ptr<SequencePoint> sequence_point_;

  std::unordered_map<ObjectReferenceImpl*,
                     std::unique_ptr<SharedObjectTransaction>>
      object_transactions_;
  std::unordered_map<ObjectReferenceImpl*, std::shared_ptr<LiveObject>>
      modified_objects_;

  int transaction_level_;

  DISALLOW_COPY_AND_ASSIGN(PendingTransaction);
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_PENDING_TRANSACTION_H_
