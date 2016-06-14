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

#ifndef ENGINE_TRANSACTION_STORE_INTERNAL_INTERFACE_H_
#define ENGINE_TRANSACTION_STORE_INTERNAL_INTERFACE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace floating_temple {
namespace engine {

class LiveObject;
class ObjectReferenceImpl;
class PendingEvent;
class SequencePoint;
class TransactionId;

class TransactionStoreInternalInterface {
 public:
  enum ExecutionPhase { NORMAL, REWIND, RESUME };

  virtual ~TransactionStoreInternalInterface() {}

  // The caller must take ownership of the returned SequencePoint instance.
  virtual SequencePoint* GetCurrentSequencePoint() const = 0;

  virtual std::shared_ptr<const LiveObject> GetLiveObjectAtSequencePoint(
      ObjectReferenceImpl* object_reference,
      const SequencePoint* sequence_point, bool wait) = 0;

  virtual ObjectReferenceImpl* CreateUnboundObjectReference() = 0;
  virtual ObjectReferenceImpl* CreateBoundObjectReference(
      const std::string& name) = 0;

  virtual void CreateTransaction(
      const std::vector<std::unique_ptr<PendingEvent>>& events,
      TransactionId* transaction_id,
      const std::unordered_map<ObjectReferenceImpl*,
                               std::shared_ptr<LiveObject>>& modified_objects,
      const SequencePoint* prev_sequence_point) = 0;

  virtual bool ObjectsAreIdentical(const ObjectReferenceImpl* a,
                                   const ObjectReferenceImpl* b) const = 0;

  virtual ExecutionPhase GetExecutionPhase(
      const TransactionId& base_transaction_id) = 0;
  virtual void WaitForRewind() = 0;
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_TRANSACTION_STORE_INTERNAL_INTERFACE_H_
