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

#include "base/linked_ptr.h"
#include "base/macros.h"
#include "engine/transaction_store_internal_interface.h"
#include "third_party/gmock-1.7.0/include/gmock/gmock.h"

namespace floating_temple {
namespace engine {

class LiveObject;
class ObjectReferenceImpl;
class PendingEvent;
class SequencePoint;
class TransactionId;

class MockTransactionStoreCore {
 public:
  MockTransactionStoreCore() {}

  MOCK_CONST_METHOD0(GetCurrentSequencePoint, SequencePoint*());
  MOCK_CONST_METHOD3(
      GetLiveObjectAtSequencePoint,
      std::shared_ptr<const LiveObject>(ObjectReferenceImpl* object_reference,
                                        const SequencePoint* sequence_point,
                                        bool wait));
  MOCK_CONST_METHOD1(CreateUnboundObjectReference, void(bool versioned));
  MOCK_CONST_METHOD2(CreateBoundObjectReference,
                     void(const std::string& name, bool versioned));
  MOCK_CONST_METHOD4(
      CreateTransaction,
      void(const std::vector<linked_ptr<PendingEvent>>& events,
           TransactionId* transaction_id,
           const std::unordered_map<ObjectReferenceImpl*,
                                    std::shared_ptr<LiveObject>>&
               modified_objects,
           const SequencePoint* prev_sequence_point));
  MOCK_CONST_METHOD2(ObjectsAreIdentical,
                     bool(const ObjectReferenceImpl* a,
                          const ObjectReferenceImpl* b));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockTransactionStoreCore);
};

class MockTransactionStore : public TransactionStoreInternalInterface {
 public:
  explicit MockTransactionStore(const MockTransactionStoreCore* core);
  ~MockTransactionStore() override;

  bool delay_object_binding() const override { return true; }
  SequencePoint* GetCurrentSequencePoint() const override;
  std::shared_ptr<const LiveObject> GetLiveObjectAtSequencePoint(
      ObjectReferenceImpl* object_reference,
      const SequencePoint* sequence_point, bool wait) override;
  ObjectReferenceImpl* CreateUnboundObjectReference(bool versioned) override;
  ObjectReferenceImpl* CreateBoundObjectReference(const std::string& name,
                                                  bool versioned) override;
  void CreateTransaction(
      const std::vector<linked_ptr<PendingEvent>>& events,
      TransactionId* transaction_id,
      const std::unordered_map<ObjectReferenceImpl*,
                               std::shared_ptr<LiveObject>>& modified_objects,
      const SequencePoint* prev_sequence_point) override;
  bool ObjectsAreIdentical(const ObjectReferenceImpl* a,
                           const ObjectReferenceImpl* b) const override;

 private:
  const MockTransactionStoreCore* const core_;

  std::vector<linked_ptr<ObjectReferenceImpl>> unnamed_objects_;
  std::unordered_map<std::string, linked_ptr<ObjectReferenceImpl>>
      named_objects_;

  DISALLOW_COPY_AND_ASSIGN(MockTransactionStore);
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_MOCK_TRANSACTION_STORE_H_
