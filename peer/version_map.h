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

#ifndef PEER_VERSION_MAP_H_
#define PEER_VERSION_MAP_H_

#include <cstddef>
#include <string>
#include <tr1/unordered_map>
#include <utility>

#include "base/escape.h"
#include "base/logging.h"
#include "base/string_printf.h"
#include "peer/canonical_peer.h"
#include "peer/proto/transaction_id.pb.h"
#include "peer/transaction_id_util.h"

namespace floating_temple {
namespace peer {

class CanonicalPeer;

template<class CompareFunction>
class VersionMap {
 public:
  VersionMap();
  VersionMap(const VersionMap<CompareFunction>& other);
  ~VersionMap();

  const std::tr1::unordered_map<const CanonicalPeer*, TransactionId>&
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

  std::string Dump() const;

  VersionMap<CompareFunction>& operator=(
      const VersionMap<CompareFunction>& other);

 private:
  std::tr1::unordered_map<const CanonicalPeer*, TransactionId>
      peer_transaction_ids_;
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
  CHECK(canonical_peer != NULL);
  CHECK(transaction_id != NULL);

  const std::tr1::unordered_map<const CanonicalPeer*, TransactionId>::
      const_iterator it = peer_transaction_ids_.find(canonical_peer);

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
  CHECK(canonical_peer != NULL);

  const std::tr1::unordered_map<const CanonicalPeer*, TransactionId>::
      const_iterator it = peer_transaction_ids_.find(canonical_peer);

  if (it == peer_transaction_ids_.end()) {
    return false;
  }

  return !CompareFunction()(min_transaction_id, it->second);
}

template<class CompareFunction>
void VersionMap<CompareFunction>::AddPeerTransactionId(
    const CanonicalPeer* canonical_peer, const TransactionId& transaction_id) {
  CHECK(canonical_peer != NULL);
  CHECK(IsValidTransactionId(transaction_id)) << transaction_id.DebugString();

  const std::pair<
      std::tr1::unordered_map<const CanonicalPeer*, TransactionId>::iterator,
      bool> insert_result =
      peer_transaction_ids_.insert(std::make_pair(canonical_peer,
                                                  transaction_id));

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
  CHECK(canonical_peer != NULL);
  CHECK(IsValidTransactionId(transaction_id)) << transaction_id.DebugString();

  const std::tr1::unordered_map<const CanonicalPeer*, TransactionId>::iterator
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
  CHECK(other != NULL);
  peer_transaction_ids_.swap(other->peer_transaction_ids_);
}

template<class CompareFunction>
std::string VersionMap<CompareFunction>::Dump() const {
  std::string peer_transaction_ids_string;

  if (peer_transaction_ids_.empty()) {
    peer_transaction_ids_string = "{}";
  } else {
    peer_transaction_ids_string = "{";

    for (std::tr1::unordered_map<const CanonicalPeer*, TransactionId>::
             const_iterator it = peer_transaction_ids_.begin();
         it != peer_transaction_ids_.end(); ++it) {
      if (it != peer_transaction_ids_.begin()) {
        peer_transaction_ids_string += ",";
      }

      StringAppendF(&peer_transaction_ids_string, " \"%s\": \"%s\"",
                    CEscape(it->first->peer_id()).c_str(),
                    TransactionIdToString(it->second).c_str());
    }

    peer_transaction_ids_string += " }";
  }

  return peer_transaction_ids_string;
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
  const std::tr1::unordered_map<const CanonicalPeer*, TransactionId>& a_map =
      a.peer_transaction_ids();
  const std::tr1::unordered_map<const CanonicalPeer*, TransactionId>& b_map =
      b.peer_transaction_ids();

  if (a_map.size() != b_map.size()) {
    return false;
  }

  for (std::tr1::unordered_map<const CanonicalPeer*, TransactionId>::
           const_iterator a_it = a_map.begin();
       a_it != a_map.end(); ++a_it) {
    const std::tr1::unordered_map<const CanonicalPeer*, TransactionId>::
        const_iterator b_it = b_map.find(a_it->first);

    if (b_it == b_map.end() ||
        CompareTransactionIds(a_it->second, b_it->second) != 0) {
      return false;
    }
  }

  return true;
}

template<class CompareFunction>
bool VersionMapIsLessThanOrEqual(const VersionMap<CompareFunction>& a,
                                 const VersionMap<CompareFunction>& b) {
  const std::tr1::unordered_map<const CanonicalPeer*, TransactionId>& a_map =
      a.peer_transaction_ids();
  const std::tr1::unordered_map<const CanonicalPeer*, TransactionId>& b_map =
      b.peer_transaction_ids();

  for (std::tr1::unordered_map<const CanonicalPeer*, TransactionId>::
           const_iterator a_it = a_map.begin();
       a_it != a_map.end(); ++a_it) {
    const std::tr1::unordered_map<const CanonicalPeer*, TransactionId>::
        const_iterator b_it = b_map.find(a_it->first);

    if (b_it == b_map.end() ||
        CompareTransactionIds(a_it->second, b_it->second) > 0) {
      return false;
    }
  }

  return true;
}

template<class CompareFunction>
void GetVersionMapUnion(const VersionMap<CompareFunction>& a,
                        const VersionMap<CompareFunction>& b,
                        VersionMap<CompareFunction>* out) {
  CHECK(out != NULL);

  out->CopyFrom(a);

  const std::tr1::unordered_map<const CanonicalPeer*, TransactionId>& b_map =
      b.peer_transaction_ids();

  for (std::tr1::unordered_map<const CanonicalPeer*, TransactionId>::
           const_iterator it = b_map.begin();
       it != b_map.end(); ++it) {
    out->AddPeerTransactionId(it->first, it->second);
  }
}

template<class CompareFunction>
void GetVersionMapIntersection(const VersionMap<CompareFunction>& a,
                               const VersionMap<CompareFunction>& b,
                               VersionMap<CompareFunction>* out) {
  CHECK(out != NULL);

  const CompareFunction compare_function;

  const std::tr1::unordered_map<const CanonicalPeer*, TransactionId>& a_map =
      a.peer_transaction_ids();
  const std::tr1::unordered_map<const CanonicalPeer*, TransactionId>& b_map =
      b.peer_transaction_ids();

  for (std::tr1::unordered_map<const CanonicalPeer*, TransactionId>::
           const_iterator a_it = a_map.begin();
       a_it != a_map.end(); ++a_it) {
    const CanonicalPeer* const canonical_peer = a_it->first;

    const std::tr1::unordered_map<const CanonicalPeer*, TransactionId>::
        const_iterator b_it = b_map.find(canonical_peer);

    if (b_it != b_map.end()) {
      const TransactionId& a_transaction_id = a_it->second;
      const TransactionId& b_transaction_id = b_it->second;

      const TransactionId* transaction_id = NULL;
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

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_VERSION_MAP_H_
