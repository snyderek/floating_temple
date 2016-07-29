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

#include "engine/mock_transaction_store.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/logging.h"
#include "engine/object_reference_impl.h"
#include "engine/proto/transaction_id.pb.h"

using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;

namespace floating_temple {
namespace engine {

MockTransactionStore::MockTransactionStore(MockTransactionStoreCore* core)
    : core_(CHECK_NOTNULL(core)),
      next_id_(1) {
}

MockTransactionStore::~MockTransactionStore() {
}

const CanonicalPeer* MockTransactionStore::GetLocalPeer() const {
  return core_->GetLocalPeer();
}

SequencePoint* MockTransactionStore::GetCurrentSequencePoint() const {
  return core_->GetCurrentSequencePoint();
}

shared_ptr<const LiveObject> MockTransactionStore::GetLiveObjectAtSequencePoint(
    ObjectReferenceImpl* object_reference, const SequencePoint* sequence_point,
    bool wait) {
  CHECK(object_reference != nullptr);

  return core_->GetLiveObjectAtSequencePoint(object_reference, sequence_point,
                                             wait);
}

ObjectReferenceImpl* MockTransactionStore::CreateUnboundObjectReference() {
  core_->CreateUnboundObjectReference();

  ObjectReferenceImpl* const object_reference = new ObjectReferenceImpl();
  unnamed_objects_.emplace_back(object_reference);
  return object_reference;
}

ObjectReferenceImpl* MockTransactionStore::CreateBoundObjectReference(
    const string& name) {
  core_->CreateBoundObjectReference(name);

  unique_ptr<ObjectReferenceImpl>* object_reference = nullptr;

  if (name.empty()) {
    unnamed_objects_.emplace_back(nullptr);
    object_reference = &unnamed_objects_.back();
  } else {
    object_reference = &named_objects_[name];
  }

  if (object_reference->get() == nullptr) {
    object_reference->reset(new ObjectReferenceImpl());
  }

  return object_reference->get();
}

void MockTransactionStore::CreateTransaction(
    const vector<unique_ptr<PendingEvent>>& events,
    TransactionId* transaction_id,
    const unordered_map<ObjectReferenceImpl*, shared_ptr<LiveObject>>&
        modified_objects,
    const SequencePoint* prev_sequence_point) {
  CHECK(transaction_id != nullptr);

  core_->CreateTransaction(events, transaction_id, modified_objects,
                           prev_sequence_point);

  transaction_id->Clear();
  transaction_id->set_a(next_id_);
  transaction_id->set_b(0);
  transaction_id->set_c(0);
  ++next_id_;
}

bool MockTransactionStore::ObjectsAreIdentical(
    const ObjectReferenceImpl* a, const ObjectReferenceImpl* b) const {
  return core_->ObjectsAreIdentical(a, b);
}

TransactionStoreInternalInterface::ExecutionPhase
MockTransactionStore::GetExecutionPhase(
    const TransactionId& base_transaction_id) {
  return core_->GetExecutionPhase(base_transaction_id);
}

void MockTransactionStore::WaitForRewind() {
  core_->WaitForRewind();
}

}  // namespace engine
}  // namespace floating_temple
