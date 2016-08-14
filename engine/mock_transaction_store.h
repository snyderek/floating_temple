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

#ifndef ENGINE_MOCK_TRANSACTION_STORE_H_
#define ENGINE_MOCK_TRANSACTION_STORE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/integral_types.h"
#include "base/macros.h"
#include "engine/transaction_store_internal_interface.h"
#include "third_party/gmock-1.7.0/include/gmock/gmock.h"

namespace floating_temple {
namespace engine {

class CanonicalPeer;
class LiveObject;
class ObjectReferenceImpl;
class SequencePoint;
class SharedObjectTransaction;
class TransactionId;

class MockTransactionStoreCore {
 public:
  MockTransactionStoreCore() {}

  MOCK_CONST_METHOD0(GetLocalPeer, const CanonicalPeer*());
  MOCK_CONST_METHOD0(GetCurrentSequencePoint, void());
  MOCK_CONST_METHOD3(GetLiveObjectAtSequencePoint,
                     void(ObjectReferenceImpl* object_reference,
                          const SequencePoint* sequence_point, bool wait));
  MOCK_CONST_METHOD0(CreateUnboundObjectReference, void());
  MOCK_CONST_METHOD1(CreateBoundObjectReference, void(const std::string& name));
  MOCK_CONST_METHOD4(
      CreateTransaction,
      void(const std::unordered_map<ObjectReferenceImpl*,
                                    std::unique_ptr<SharedObjectTransaction>>&
               object_transactions,
           TransactionId* transaction_id,
           const std::unordered_map<ObjectReferenceImpl*,
                                    std::shared_ptr<LiveObject>>&
               modified_objects,
           const SequencePoint* prev_sequence_point));
  MOCK_CONST_METHOD2(ObjectsAreIdentical,
                     bool(const ObjectReferenceImpl* a,
                          const ObjectReferenceImpl* b));
  MOCK_METHOD1(GetExecutionPhase,
               TransactionStoreInternalInterface::ExecutionPhase(
                   const TransactionId& base_transaction_id));
  MOCK_METHOD0(WaitForRewind, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockTransactionStoreCore);
};

class MockTransactionStore : public TransactionStoreInternalInterface {
 public:
  explicit MockTransactionStore(MockTransactionStoreCore* core);
  ~MockTransactionStore() override;

  const CanonicalPeer* GetLocalPeer() const override;
  SequencePoint* GetCurrentSequencePoint() const override;
  std::shared_ptr<const LiveObject> GetLiveObjectAtSequencePoint(
      ObjectReferenceImpl* object_reference,
      const SequencePoint* sequence_point, bool wait) override;
  ObjectReferenceImpl* CreateUnboundObjectReference() override;
  ObjectReferenceImpl* CreateBoundObjectReference(
      const std::string& name) override;
  void CreateTransaction(
      const std::unordered_map<ObjectReferenceImpl*,
                               std::unique_ptr<SharedObjectTransaction>>&
          object_transactions,
      TransactionId* transaction_id,
      const std::unordered_map<ObjectReferenceImpl*,
                               std::shared_ptr<LiveObject>>& modified_objects,
      const SequencePoint* prev_sequence_point) override;
  bool ObjectsAreIdentical(const ObjectReferenceImpl* a,
                           const ObjectReferenceImpl* b) const override;
  ExecutionPhase GetExecutionPhase(
      const TransactionId& base_transaction_id) override;
  void WaitForRewind() override;

 private:
  MockTransactionStoreCore* const core_;

  std::vector<std::unique_ptr<ObjectReferenceImpl>> unnamed_objects_;
  std::unordered_map<std::string, std::unique_ptr<ObjectReferenceImpl>>
      named_objects_;
  std::unordered_map<ObjectReferenceImpl*, std::shared_ptr<LiveObject>>
      live_objects_;
  uint64 next_transaction_id_;

  DISALLOW_COPY_AND_ASSIGN(MockTransactionStore);
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_MOCK_TRANSACTION_STORE_H_
