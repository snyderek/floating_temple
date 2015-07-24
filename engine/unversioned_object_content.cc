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

#include "engine/unversioned_object_content.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/string_printf.h"
#include "engine/live_object.h"
#include "engine/max_version_map.h"
#include "engine/transaction_id_util.h"
#include "engine/unversioned_live_object.h"

using std::map;
using std::pair;
using std::shared_ptr;
using std::string;
using std::unordered_map;
using std::vector;

namespace floating_temple {
namespace engine {

UnversionedObjectContent::UnversionedObjectContent(
    TransactionStoreInternalInterface* transaction_store,
    const shared_ptr<const LiveObject>& live_object)
    : transaction_store_(CHECK_NOTNULL(transaction_store)),
      live_object_(live_object) {
  CHECK(live_object.get() != nullptr);
}

UnversionedObjectContent::~UnversionedObjectContent() {
}

shared_ptr<const LiveObject> UnversionedObjectContent::GetWorkingVersion(
    const MaxVersionMap& transaction_store_version_map,
    const SequencePointImpl& sequence_point,
    unordered_map<SharedObject*, ObjectReferenceImpl*>* new_object_references,
    vector<pair<const CanonicalPeer*, TransactionId>>* transactions_to_reject) {
  return live_object_;
}

void UnversionedObjectContent::GetTransactions(
    const MaxVersionMap& transaction_store_version_map,
    map<TransactionId, linked_ptr<SharedObjectTransaction>>* transactions,
    MaxVersionMap* effective_version) const {
  CHECK(effective_version != nullptr);
  effective_version->CopyFrom(transaction_store_version_map);
}

void UnversionedObjectContent::StoreTransactions(
    const CanonicalPeer* remote_peer,
    const map<TransactionId, linked_ptr<SharedObjectTransaction>>& transactions,
    const MaxVersionMap& version_map,
    unordered_map<SharedObject*, ObjectReferenceImpl*>* new_object_references,
    vector<pair<const CanonicalPeer*, TransactionId>>* transactions_to_reject) {
  LOG(FATAL) << "Unversioned objects can not have transactions.";
}

void UnversionedObjectContent::InsertTransaction(
    const CanonicalPeer* origin_peer,
    const TransactionId& transaction_id,
    const vector<linked_ptr<CommittedEvent>>& events,
    unordered_map<SharedObject*, ObjectReferenceImpl*>* new_object_references,
    vector<pair<const CanonicalPeer*, TransactionId>>* transactions_to_reject) {
  LOG(FATAL) << "Unversioned objects can not have transactions.";
}

void UnversionedObjectContent::SetCachedLiveObject(
    const shared_ptr<const LiveObject>& cached_live_object,
    const SequencePointImpl& cached_sequence_point) {
}

string UnversionedObjectContent::Dump() const {
  return StringPrintf("{ \"live_object\": %s }", live_object_->Dump().c_str());
}

}  // namespace engine
}  // namespace floating_temple
