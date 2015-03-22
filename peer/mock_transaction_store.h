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

#ifndef PEER_MOCK_TRANSACTION_STORE_H_
#define PEER_MOCK_TRANSACTION_STORE_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "base/linked_ptr.h"
#include "base/macros.h"
#include "peer/const_live_object_ptr.h"
#include "peer/live_object_ptr.h"
#include "peer/transaction_store_internal_interface.h"
#include "third_party/gmock-1.7.0/include/gmock/gmock.h"

namespace floating_temple {
namespace peer {

class PeerObjectImpl;
class PendingEvent;
class SequencePoint;
class TransactionId;

class MockTransactionStoreCore {
 public:
  MockTransactionStoreCore() {}

  MOCK_CONST_METHOD0(GetCurrentSequencePoint, SequencePoint*());
  MOCK_CONST_METHOD3(GetLiveObjectAtSequencePoint,
                     ConstLiveObjectPtr(PeerObjectImpl* peer_object,
                                        const SequencePoint* sequence_point,
                                        bool wait));
  MOCK_CONST_METHOD0(CreateUnboundPeerObject, void());
  MOCK_CONST_METHOD1(GetOrCreateNamedObject, void(const std::string& name));
  MOCK_CONST_METHOD4(
      CreateTransaction,
      void(const std::vector<linked_ptr<PendingEvent>>& events,
           TransactionId* transaction_id,
           const std::unordered_map<PeerObjectImpl*, LiveObjectPtr>&
               modified_objects,
           const SequencePoint* prev_sequence_point));
  MOCK_CONST_METHOD2(ObjectsAreEquivalent,
                     bool(const PeerObjectImpl* a, const PeerObjectImpl* b));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockTransactionStoreCore);
};

class MockTransactionStore : public TransactionStoreInternalInterface {
 public:
  explicit MockTransactionStore(const MockTransactionStoreCore* core);
  ~MockTransactionStore() override;

  bool delay_object_binding() const override { return true; }
  SequencePoint* GetCurrentSequencePoint() const override;
  ConstLiveObjectPtr GetLiveObjectAtSequencePoint(
      PeerObjectImpl* peer_object, const SequencePoint* sequence_point,
      bool wait) override;
  PeerObjectImpl* CreateUnboundPeerObject() override;
  PeerObjectImpl* GetOrCreateNamedObject(const std::string& name) override;
  void CreateTransaction(
      const std::vector<linked_ptr<PendingEvent>>& events,
      TransactionId* transaction_id,
      const std::unordered_map<PeerObjectImpl*, LiveObjectPtr>&
          modified_objects,
      const SequencePoint* prev_sequence_point) override;
  bool ObjectsAreEquivalent(const PeerObjectImpl* a,
                            const PeerObjectImpl* b) const override;

 private:
  const MockTransactionStoreCore* const core_;

  std::vector<linked_ptr<PeerObjectImpl>> unnamed_objects_;
  std::unordered_map<std::string, linked_ptr<PeerObjectImpl>> named_objects_;

  DISALLOW_COPY_AND_ASSIGN(MockTransactionStore);
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_MOCK_TRANSACTION_STORE_H_
