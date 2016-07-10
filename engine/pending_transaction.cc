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
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/escape.h"
#include "base/logging.h"
#include "engine/live_object.h"
#include "engine/pending_event.h"
#include "engine/proto/transaction_id.pb.h"
#include "engine/sequence_point.h"
#include "engine/transaction_store_internal_interface.h"

using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::unordered_set;
using std::vector;

namespace floating_temple {
namespace engine {

PendingTransaction::PendingTransaction(
    TransactionStoreInternalInterface* transaction_store,
    const TransactionId& base_transaction_id, SequencePoint* sequence_point)
    : transaction_store_(CHECK_NOTNULL(transaction_store)),
      base_transaction_id_(base_transaction_id),
      sequence_point_(CHECK_NOTNULL(sequence_point)),
      transaction_level_(0) {
}

PendingTransaction::~PendingTransaction() {
}

shared_ptr<LiveObject> PendingTransaction::GetLiveObject(
    ObjectReferenceImpl* object_reference) {
  CHECK(object_reference != nullptr);

  shared_ptr<LiveObject>& live_object = modified_objects_[object_reference];

  if (live_object.get() == nullptr) {
    const shared_ptr<const LiveObject> existing_live_object =
        transaction_store_->GetLiveObjectAtSequencePoint(object_reference,
                                                         sequence_point_.get(),
                                                         true);
    live_object = existing_live_object->Clone();
  }

  return live_object;
}

bool PendingTransaction::IsObjectKnown(ObjectReferenceImpl* object_reference) {
  const shared_ptr<const LiveObject> existing_live_object =
      transaction_store_->GetLiveObjectAtSequencePoint(object_reference,
                                                       sequence_point_.get(),
                                                       false);
  return existing_live_object.get() != nullptr;
}

bool PendingTransaction::AddNewObject(
    ObjectReferenceImpl* object_reference,
    const shared_ptr<const LiveObject>& live_object) {
  CHECK(object_reference != nullptr);
  CHECK(live_object.get() != nullptr);

  if (!new_objects_.insert(object_reference).second) {
    return false;
  }

  // Make the object available to other methods in the same transaction. (Later
  // transactions will be able to fetch the object from the transaction store.)
  const shared_ptr<LiveObject> modified_object = live_object->Clone();
  CHECK(modified_objects_.emplace(object_reference, modified_object).second);

  return true;
}

void PendingTransaction::UpdateLiveObject(
    ObjectReferenceImpl* object_reference,
    const shared_ptr<LiveObject>& live_object) {
  CHECK(object_reference != nullptr);
  CHECK(live_object.get() != nullptr);

  const auto insert_result = modified_objects_.emplace(object_reference,
                                                       live_object);
  if (!insert_result.second) {
    CHECK(insert_result.first->second.get() == live_object.get());
  }
}

void PendingTransaction::AddEvent(PendingEvent* event) {
  CHECK(event != nullptr);

  events_.emplace_back(event);
}

void PendingTransaction::IncrementTransactionLevel() {
  CHECK_GE(transaction_level_, 0);
  ++transaction_level_;
}

bool PendingTransaction::DecrementTransactionLevel() {
  CHECK_GT(transaction_level_, 0);
  --transaction_level_;
  return transaction_level_ == 0;
}

void PendingTransaction::Commit(
    TransactionId* transaction_id,
    unordered_set<ObjectReferenceImpl*>* new_objects) {
  CHECK(transaction_id != nullptr);
  CHECK(new_objects != nullptr);

  TransactionId committed_transaction_id;
  while (!events_.empty()) {
    LogDebugInfo();

    vector<unique_ptr<PendingEvent>> events_to_commit;
    unordered_map<ObjectReferenceImpl*, shared_ptr<LiveObject>>
        modified_objects_to_commit;
    events_to_commit.swap(events_);
    modified_objects_to_commit.swap(modified_objects_);

    transaction_store_->CreateTransaction(events_to_commit,
                                          &committed_transaction_id,
                                          modified_objects_to_commit,
                                          sequence_point_.get());
  }

  transaction_id->Swap(&committed_transaction_id);
  new_objects->swap(new_objects_);
}

void PendingTransaction::LogDebugInfo() const {
  const vector<unique_ptr<PendingEvent>>::size_type event_count =
      events_.size();

  VLOG(2) << "Creating local transaction with " << event_count << " events.";

  if (VLOG_IS_ON(3)) {
    for (vector<unique_ptr<PendingEvent>>::size_type i = 0; i < event_count;
         ++i) {
      const PendingEvent* const event = events_[i].get();
      const PendingEvent::Type type = event->type();
      const string type_string = PendingEvent::GetTypeString(type);

      if (type == PendingEvent::METHOD_CALL) {
        ObjectReferenceImpl* next_object_reference = nullptr;
        const string* method_name = nullptr;
        const vector<Value>* parameters = nullptr;

        event->GetMethodCall(&next_object_reference, &method_name, &parameters);

        VLOG(3) << "Event " << i << ": " << type_string << " \""
                << CEscape(*method_name) << "\"";
      } else {
        VLOG(3) << "Event " << i << ": " << type_string;
      }
    }
  }
}

}  // namespace engine
}  // namespace floating_temple
