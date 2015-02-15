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

#ifndef PEER_TRANSACTION_ID_UTIL_H_
#define PEER_TRANSACTION_ID_UTIL_H_

#include <string>

namespace floating_temple {
namespace peer {

class TransactionId;

int CompareTransactionIds(const TransactionId& a, const TransactionId& b);

void GetMinTransactionId(TransactionId* transaction_id);
void GetMaxTransactionId(TransactionId* transaction_id);

bool IsValidTransactionId(const TransactionId& transaction_id);

void IncrementTransactionId(TransactionId* transaction_id);

std::string TransactionIdToString(const TransactionId& transaction_id);

bool operator<(const TransactionId& a, const TransactionId& b);
bool operator==(const TransactionId& a, const TransactionId& b);

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_TRANSACTION_ID_UTIL_H_