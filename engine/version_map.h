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

#ifndef ENGINE_VERSION_MAP_H_
#define ENGINE_VERSION_MAP_H_

#include <unordered_map>
#include <utility>

#include "base/escape.h"
#include "base/logging.h"
#include "engine/canonical_peer.h"
#include "engine/proto/transaction_id.pb.h"
#include "engine/transaction_id_util.h"
#include "util/dump_context.h"

namespace floating_temple {
namespace engine {

class CanonicalPeer;

template<class CompareFunction>
class VersionMap {
 public:
  VersionMap();
  VersionMap(const VersionMap<CompareFunction>& other);
  ~VersionMap();

  const std::unordered_map<const CanonicalPeer*, TransactionId>&
      peer_transaction_ids() const { return peer_transaction_ids_; }

  bool IsEmpty() const { return peer_transaction_ids_.empty(); }
  void Clear();

  bool GetPeerTransactionId(const CanonicalPeer* canonical_peer,
                            TransactionId* transaction_id) const;
  // TODO(dss): Rename the parameter 'min_transaction_id'.
  bool HasPeerTransactionId(const CanonicalPeer* canonical_peer,
                            const TransactionId& min_transaction_id) const;

  void AddPeerTransactionId(const CanonicalPeer* canonical_peer,
                            const TransactionId& transaction_id);
  void RemovePeerTransactionId(const CanonicalPeer* canonical_peer,
                               const TransactionId& transaction_id);

  void CopyFrom(const VersionMap<CompareFunction>& other);
  void Swap(VersionMap<CompareFunction>* other);

  void Dump(DumpContext* dc) const;

  VersionMap<CompareFunction>& operator=(
      const VersionMap<CompareFunction>& other);

 private:
  std::unordered_map<const CanonicalPeer*, TransactionId> peer_transaction_ids_;
};

template<class CompareFunction>
VersionMap<CompareFunction>::VersionMap() {
}

template<class CompareFunction>
VersionMap<CompareFunction>::VersionMap(
    const VersionMap<CompareFunction>& other)
    : peer_transaction_ids_(other.peer_transaction_ids_) {
}

template<class CompareFunction>
VersionMap<CompareFunction>::~VersionMap() {
}

template<class CompareFunction>
void VersionMap<CompareFunction>::Clear() {
  peer_transaction_ids_.clear();
}

template<class CompareFunction>
bool VersionMap<CompareFunction>::GetPeerTransactionId(
    const CanonicalPeer* canonical_peer, TransactionId* transaction_id) const {
  CHECK(canonical_peer != nullptr);
  CHECK(transaction_id != nullptr);

  const std::unordered_map<const CanonicalPeer*, TransactionId>::const_iterator
      it = peer_transaction_ids_.find(canonical_peer);

  if (it == peer_transaction_ids_.end()) {
    return false;
  }

  transaction_id->CopyFrom(it->second);
  return true;
}

template<class CompareFunction>
bool VersionMap<CompareFunction>::HasPeerTransactionId(
    const CanonicalPeer* canonical_peer,
    const TransactionId& min_transaction_id) const {
  CHECK(canonical_peer != nullptr);

  const std::unordered_map<const CanonicalPeer*, TransactionId>::const_iterator
      it = peer_transaction_ids_.find(canonical_peer);

  if (it == peer_transaction_ids_.end()) {
    return false;
  }

  return !CompareFunction()(min_transaction_id, it->second);
}

template<class CompareFunction>
void VersionMap<CompareFunction>::AddPeerTransactionId(
    const CanonicalPeer* canonical_peer, const TransactionId& transaction_id) {
  CHECK(canonical_peer != nullptr);
  CHECK(IsValidTransactionId(transaction_id)) << transaction_id.DebugString();

  const auto insert_result = peer_transaction_ids_.emplace(canonical_peer,
                                                           transaction_id);

  if (!insert_result.second) {
    TransactionId* const existing_transaction_id = &insert_result.first->second;

    if (!CompareFunction()(transaction_id, *existing_transaction_id)) {
      return;
    }

    existing_transaction_id->CopyFrom(transaction_id);
  }
}

template<class CompareFunction>
void VersionMap<CompareFunction>::RemovePeerTransactionId(
    const CanonicalPeer* canonical_peer, const TransactionId& transaction_id) {
  CHECK(canonical_peer != nullptr);
  CHECK(IsValidTransactionId(transaction_id)) << transaction_id.DebugString();

  const std::unordered_map<const CanonicalPeer*, TransactionId>::iterator
      peer_it = peer_transaction_ids_.find(canonical_peer);

  if (peer_it == peer_transaction_ids_.end()) {
    return;
  }

  TransactionId* const existing_transaction_id = &peer_it->second;

  if (CompareFunction()(*existing_transaction_id, transaction_id)) {
    return;
  }

  TransactionId deleted_transaction_id;
  deleted_transaction_id.Swap(existing_transaction_id);

  peer_transaction_ids_.erase(peer_it);
}

template<class CompareFunction>
void VersionMap<CompareFunction>::CopyFrom(
    const VersionMap<CompareFunction>& other) {
  peer_transaction_ids_ = other.peer_transaction_ids_;
}

template<class CompareFunction>
void VersionMap<CompareFunction>::Swap(VersionMap<CompareFunction>* other) {
  CHECK(other != nullptr);
  peer_transaction_ids_.swap(other->peer_transaction_ids_);
}

template<class CompareFunction>
void VersionMap<CompareFunction>::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();
  for (const std::pair<const CanonicalPeer*, TransactionId>&
           peer_transaction_id : peer_transaction_ids_) {
    dc->AddString(peer_transaction_id.first->peer_id());
    dc->AddString(TransactionIdToString(peer_transaction_id.second));
  }
  dc->End();
}

template<class CompareFunction>
VersionMap<CompareFunction>& VersionMap<CompareFunction>::operator=(
    const VersionMap<CompareFunction>& other) {
  peer_transaction_ids_ = other.peer_transaction_ids_;
  return *this;
}

template<class CompareFunction>
bool VersionMapsAreEqual(const VersionMap<CompareFunction>& a,
                         const VersionMap<CompareFunction>& b) {
  const std::unordered_map<const CanonicalPeer*, TransactionId>& a_map =
      a.peer_transaction_ids();
  const std::unordered_map<const CanonicalPeer*, TransactionId>& b_map =
      b.peer_transaction_ids();

  if (a_map.size() != b_map.size()) {
    return false;
  }

  for (const auto& a_pair : a_map) {
    const std::unordered_map<const CanonicalPeer*, TransactionId>::
        const_iterator b_it = b_map.find(a_pair.first);

    if (b_it == b_map.end() ||
        CompareTransactionIds(a_pair.second, b_it->second) != 0) {
      return false;
    }
  }

  return true;
}

template<class CompareFunction>
bool VersionMapIsLessThanOrEqual(const VersionMap<CompareFunction>& a,
                                 const VersionMap<CompareFunction>& b) {
  const std::unordered_map<const CanonicalPeer*, TransactionId>& a_map =
      a.peer_transaction_ids();
  const std::unordered_map<const CanonicalPeer*, TransactionId>& b_map =
      b.peer_transaction_ids();

  for (const auto& a_pair : a_map) {
    const std::unordered_map<const CanonicalPeer*, TransactionId>::
        const_iterator b_it = b_map.find(a_pair.first);

    if (b_it == b_map.end() ||
        CompareTransactionIds(a_pair.second, b_it->second) > 0) {
      return false;
    }
  }

  return true;
}

template<class CompareFunction>
void GetVersionMapUnion(const VersionMap<CompareFunction>& a,
                        const VersionMap<CompareFunction>& b,
                        VersionMap<CompareFunction>* out) {
  CHECK(out != nullptr);

  out->CopyFrom(a);
  for (const auto& b_pair : b.peer_transaction_ids()) {
    out->AddPeerTransactionId(b_pair.first, b_pair.second);
  }
}

template<class CompareFunction>
void GetVersionMapIntersection(const VersionMap<CompareFunction>& a,
                               const VersionMap<CompareFunction>& b,
                               VersionMap<CompareFunction>* out) {
  CHECK(out != nullptr);

  const CompareFunction compare_function;

  const std::unordered_map<const CanonicalPeer*, TransactionId>& a_map =
      a.peer_transaction_ids();
  const std::unordered_map<const CanonicalPeer*, TransactionId>& b_map =
      b.peer_transaction_ids();

  for (const auto& a_pair : a_map) {
    const CanonicalPeer* const canonical_peer = a_pair.first;

    const std::unordered_map<const CanonicalPeer*, TransactionId>::
        const_iterator b_it = b_map.find(canonical_peer);

    if (b_it != b_map.end()) {
      const TransactionId& a_transaction_id = a_pair.second;
      const TransactionId& b_transaction_id = b_it->second;

      const TransactionId* transaction_id = nullptr;
      if (compare_function(a_transaction_id, b_transaction_id)) {
        transaction_id = &a_transaction_id;
      } else {
        transaction_id = &b_transaction_id;
      }

      out->AddPeerTransactionId(canonical_peer, *transaction_id);
    }
  }
}

template<class CompareFunction>
bool operator==(const VersionMap<CompareFunction>& a,
                const VersionMap<CompareFunction>& b) {
  return VersionMapsAreEqual(a, b);
}

template<class CompareFunction>
bool operator!=(const VersionMap<CompareFunction>& a,
                const VersionMap<CompareFunction>& b) {
  return !(a == b);
}

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_VERSION_MAP_H_
