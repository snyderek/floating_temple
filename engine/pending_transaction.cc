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

#include "engine/pending_transaction.h"

#include <memory>
#include <unordered_map>
#include <vector>

#include "base/logging.h"
#include "engine/live_object.h"
#include "engine/pending_event.h"
#include "engine/proto/transaction_id.pb.h"
#include "engine/sequence_point.h"
#include "engine/transaction_store_internal_interface.h"

using std::shared_ptr;

namespace floating_temple {
namespace engine {

PendingTransaction::PendingTransaction(
    TransactionStoreInternalInterface* transaction_store,
    const TransactionId& base_transaction_id)
    : transaction_store_(CHECK_NOTNULL(transaction_store)),
      base_transaction_id_(base_transaction_id),
      committed_(false) {
}

PendingTransaction::~PendingTransaction() {
}

shared_ptr<LiveObject> PendingTransaction::GetLiveObject(
    ObjectReferenceImpl* object_reference) {
  CHECK(!committed_);
  CHECK(object_reference != nullptr);

  shared_ptr<LiveObject>& live_object = modified_objects_[object_reference];

  if (live_object.get() == nullptr) {
    const shared_ptr<const LiveObject> existing_live_object =
        transaction_store_->GetLiveObjectAtSequencePoint(object_reference,
                                                         GetSequencePoint(),
                                                         true);
    live_object = existing_live_object->Clone();
  }

  return live_object;
}

bool PendingTransaction::IsObjectKnown(ObjectReferenceImpl* object_reference) {
  const shared_ptr<const LiveObject> existing_live_object =
      transaction_store_->GetLiveObjectAtSequencePoint(object_reference,
                                                       GetSequencePoint(),
                                                       false);
  return existing_live_object.get() != nullptr;
}

void PendingTransaction::AddLiveObject(
    ObjectReferenceImpl* object_reference,
    const shared_ptr<LiveObject>& live_object) {
  CHECK(!committed_);
  CHECK(object_reference != nullptr);
  CHECK(live_object.get() != nullptr);

  const auto insert_result = modified_objects_.emplace(object_reference,
                                                       live_object);
  if (!insert_result.second) {
    CHECK(insert_result.first->second.get() == live_object.get());
  }
}

void PendingTransaction::AddEvent(PendingEvent* event) {
  CHECK(!committed_);
  CHECK(event != nullptr);

  events_.emplace_back(event);
}

void PendingTransaction::Commit(TransactionId* transaction_id) {
  CHECK(!committed_);

  if (!events_.empty()) {
    transaction_store_->CreateTransaction(events_, transaction_id,
                                          modified_objects_,
                                          GetSequencePoint());
  }

  committed_ = true;
}

const SequencePoint* PendingTransaction::GetSequencePoint() {
  if (sequence_point_.get() == nullptr) {
    sequence_point_.reset(transaction_store_->GetCurrentSequencePoint());
  }
  return sequence_point_.get();
}

}  // namespace engine
}  // namespace floating_temple
