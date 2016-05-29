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

#ifndef ENGINE_TRANSACTION_ID_UTIL_H_
#define ENGINE_TRANSACTION_ID_UTIL_H_

#include <ostream>
#include <string>

namespace floating_temple {
namespace engine {

class TransactionId;

extern const TransactionId& MIN_TRANSACTION_ID;
extern const TransactionId& MAX_TRANSACTION_ID;

int CompareTransactionIds(const TransactionId& a, const TransactionId& b);
bool IsValidTransactionId(const TransactionId& transaction_id);
std::string TransactionIdToString(const TransactionId& transaction_id);

std::ostream& operator<<(std::ostream& os, const TransactionId& transaction_id);

inline bool operator<(const TransactionId& a, const TransactionId& b) {
  return CompareTransactionIds(a, b) < 0;
}

inline bool operator==(const TransactionId& a, const TransactionId& b) {
  return CompareTransactionIds(a, b) == 0;
}

inline bool operator>(const TransactionId& a, const TransactionId& b) {
  return b < a;
}

inline bool operator<=(const TransactionId& a, const TransactionId& b) {
  return !(b < a);
}

inline bool operator>=(const TransactionId& a, const TransactionId& b) {
  return !(a < b);
}

inline bool operator!=(const TransactionId& a, const TransactionId& b) {
  return !(a == b);
}

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_TRANSACTION_ID_UTIL_H_
