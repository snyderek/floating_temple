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
#include <utility>
#include <vector>

#include "base/logging.h"
#include "engine/committed_event.h"
#include "engine/mock_sequence_point.h"
#include "engine/object_reference_impl.h"
#include "engine/proto/transaction_id.pb.h"
#include "engine/proto/uuid.pb.h"
#include "engine/shared_object.h"
#include "engine/shared_object_transaction.h"

using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;

namespace floating_temple {
namespace engine {

MockTransactionStore::MockTransactionStore(MockTransactionStoreCore* core)
    : core_(CHECK_NOTNULL(core)),
      next_object_id_(1),
      next_transaction_id_(1) {
}

MockTransactionStore::~MockTransactionStore() {
}

const CanonicalPeer* MockTransactionStore::GetLocalPeer() const {
  return core_->GetLocalPeer();
}

SequencePoint* MockTransactionStore::GetCurrentSequencePoint() const {
  core_->GetCurrentSequencePoint();
  return new MockSequencePoint();
}

shared_ptr<const LiveObject> MockTransactionStore::GetLiveObjectAtSequencePoint(
    ObjectReferenceImpl* object_reference, const SequencePoint* sequence_point,
    bool wait) {
  CHECK(object_reference != nullptr);

  core_->GetLiveObjectAtSequencePoint(object_reference, sequence_point, wait);

  const auto it = live_objects_.find(object_reference);
  CHECK(it != live_objects_.end());
  return it->second;
}

ObjectReferenceImpl* MockTransactionStore::CreateObjectReference(
    const string& name) {
  core_->CreateObjectReference(name);

  unique_ptr<ObjectReferenceImpl>* object_reference = nullptr;

  if (name.empty()) {
    unnamed_objects_.emplace_back(nullptr);
    object_reference = &unnamed_objects_.back();
  } else {
    object_reference = &named_objects_[name];
  }

  if (object_reference->get() == nullptr) {
    Uuid object_id;
    object_id.set_high_word(0);
    object_id.set_low_word(next_object_id_);
    ++next_object_id_;

    SharedObject* const shared_object = new SharedObject(this, object_id);
    shared_objects_.emplace_back(shared_object);
    object_reference->reset(new ObjectReferenceImpl(shared_object));
    shared_object->AddObjectReference(object_reference->get());
  }

  return object_reference->get();
}

void MockTransactionStore::CreateTransaction(
    const unordered_map<ObjectReferenceImpl*,
                        unique_ptr<SharedObjectTransaction>>&
        object_transactions,
    TransactionId* transaction_id,
    const unordered_map<ObjectReferenceImpl*, shared_ptr<LiveObject>>&
        modified_objects,
    const SequencePoint* prev_sequence_point) {
  CHECK(transaction_id != nullptr);

  if (VLOG_IS_ON(3)) {
    int shared_object_index = 0;
    for (const auto& transaction_pair : object_transactions) {
      const ObjectReferenceImpl* const object_reference =
          transaction_pair.first;
      const SharedObjectTransaction* const shared_object_transaction =
          transaction_pair.second.get();

      VLOG(3) << "Shared object " << shared_object_index << ": "
              << object_reference->DebugString();

      const vector<unique_ptr<CommittedEvent>>& events =
          shared_object_transaction->events();
      const vector<unique_ptr<CommittedEvent>>::size_type event_count =
          events.size();

      for (vector<unique_ptr<CommittedEvent>>::size_type event_index = 0;
           event_index < event_count; ++event_index) {
        VLOG(3) << "Event " << event_index << ": "
                << events[event_index]->DebugString();
      }

      ++shared_object_index;
    }
  }

  core_->CreateTransaction(object_transactions, transaction_id,
                           modified_objects, prev_sequence_point);

  for (const auto& object_pair : modified_objects) {
    live_objects_[object_pair.first] = object_pair.second;
  }

  transaction_id->Clear();
  transaction_id->set_a(next_transaction_id_);
  transaction_id->set_b(0);
  transaction_id->set_c(0);
  ++next_transaction_id_;
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
