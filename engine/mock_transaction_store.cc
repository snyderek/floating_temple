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

#include "base/linked_ptr.h"
#include "base/logging.h"
#include "engine/object_reference_impl.h"

using std::shared_ptr;
using std::string;
using std::unordered_map;
using std::vector;

namespace floating_temple {
namespace peer {

MockTransactionStore::MockTransactionStore(
    const MockTransactionStoreCore* core)
    : core_(CHECK_NOTNULL(core)) {
}

MockTransactionStore::~MockTransactionStore() {
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

ObjectReferenceImpl* MockTransactionStore::CreateUnboundObjectReference(
    bool versioned) {
  core_->CreateUnboundObjectReference(versioned);

  ObjectReferenceImpl* const object_reference = new ObjectReferenceImpl(
      versioned);
  unnamed_objects_.emplace_back(object_reference);
  return object_reference;
}

ObjectReferenceImpl* MockTransactionStore::CreateBoundObjectReference(
    const string& name, bool versioned) {
  core_->CreateBoundObjectReference(name, versioned);

  linked_ptr<ObjectReferenceImpl>* object_reference = nullptr;

  if (name.empty()) {
    unnamed_objects_.emplace_back(nullptr);
    object_reference = &unnamed_objects_.back();
  } else {
    object_reference = &named_objects_[name];
  }

  if (object_reference->get() == nullptr) {
    object_reference->reset(new ObjectReferenceImpl(versioned));
  }

  return object_reference->get();
}

void MockTransactionStore::CreateTransaction(
    const vector<linked_ptr<PendingEvent>>& events,
    TransactionId* transaction_id,
    const unordered_map<ObjectReferenceImpl*, shared_ptr<LiveObject>>&
        modified_objects,
    const SequencePoint* prev_sequence_point) {
  core_->CreateTransaction(events, transaction_id, modified_objects,
                           prev_sequence_point);
}

bool MockTransactionStore::ObjectsAreIdentical(
    const ObjectReferenceImpl* a, const ObjectReferenceImpl* b) const {
  return core_->ObjectsAreIdentical(a, b);
}

}  // namespace peer
}  // namespace floating_temple
