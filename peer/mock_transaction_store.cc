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

#include "peer/mock_transaction_store.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/linked_ptr.h"
#include "base/logging.h"
#include "peer/peer_object_impl.h"

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
    PeerObjectImpl* peer_object, const SequencePoint* sequence_point,
    bool wait) {
  CHECK(peer_object != nullptr);

  return core_->GetLiveObjectAtSequencePoint(peer_object, sequence_point, wait);
}

PeerObjectImpl* MockTransactionStore::CreateUnboundPeerObject(bool versioned) {
  core_->CreateUnboundPeerObject(versioned);

  PeerObjectImpl* const peer_object = new PeerObjectImpl(versioned);
  unnamed_objects_.emplace_back(peer_object);
  return peer_object;
}

PeerObjectImpl* MockTransactionStore::CreateBoundPeerObject(const string& name,
                                                            bool versioned) {
  core_->CreateBoundPeerObject(name, versioned);

  linked_ptr<PeerObjectImpl>* peer_object = nullptr;

  if (name.empty()) {
    unnamed_objects_.emplace_back(nullptr);
    peer_object = &unnamed_objects_.back();
  } else {
    peer_object = &named_objects_[name];
  }

  if (peer_object->get() == nullptr) {
    peer_object->reset(new PeerObjectImpl(versioned));
  }

  return peer_object->get();
}

void MockTransactionStore::CreateTransaction(
    const vector<linked_ptr<PendingEvent>>& events,
    TransactionId* transaction_id,
    const unordered_map<PeerObjectImpl*, shared_ptr<LiveObject>>&
        modified_objects,
    const SequencePoint* prev_sequence_point) {
  core_->CreateTransaction(events, transaction_id, modified_objects,
                           prev_sequence_point);
}

bool MockTransactionStore::ObjectsAreEquivalent(const PeerObjectImpl* a,
                                                const PeerObjectImpl* b) const {
  return core_->ObjectsAreEquivalent(a, b);
}

}  // namespace peer
}  // namespace floating_temple
