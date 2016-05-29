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

#include "engine/transaction_id_util.h"

#include <cinttypes>
#include <ostream>
#include <string>

#include "base/integral_types.h"
#include "base/logging.h"
#include "base/string_printf.h"
#include "engine/proto/transaction_id.pb.h"

using std::ostream;
using std::string;

namespace floating_temple {
namespace engine {

const TransactionId& MIN_TRANSACTION_ID = *[]{
  TransactionId* const transaction_id = new TransactionId();
  transaction_id->set_a(0);
  transaction_id->set_b(0);
  transaction_id->set_c(0);
  return transaction_id;
}();

const TransactionId& MAX_TRANSACTION_ID = *[]{
  TransactionId* const transaction_id = new TransactionId();
  transaction_id->set_a(kUint64Max);
  transaction_id->set_b(kUint64Max);
  transaction_id->set_c(kUint64Max);
  return transaction_id;
}();

int CompareTransactionIds(const TransactionId& t1, const TransactionId& t2) {
  if (t1.a() != t2.a()) {
    return t1.a() < t2.a() ? -1 : 1;
  }

  if (t1.b() != t2.b()) {
    return t1.b() < t2.b() ? -1 : 1;
  }

  if (t1.c() != t2.c()) {
    return t1.c() < t2.c() ? -1 : 1;
  }

  return 0;
}

bool IsValidTransactionId(const TransactionId& transaction_id) {
  return transaction_id.a() > 0 && transaction_id.a() < kUint64Max;
}

void IncrementTransactionId(TransactionId* transaction_id) {
  CHECK(transaction_id != nullptr);

  uint64 a = transaction_id->a();
  uint64 b = transaction_id->b();
  uint64 c = transaction_id->c();

  if (c < kUint64Max) {
    ++c;
  } else {
    c = 0;

    if (b < kUint64Max) {
      ++b;
    } else {
      b = 0;

      CHECK_LT(a, kUint64Max);
      ++a;
    }
  }

  transaction_id->set_a(a);
  transaction_id->set_b(b);
  transaction_id->set_c(c);
}

string TransactionIdToString(const TransactionId& transaction_id) {
  return StringPrintf("%016" PRIx64 "%016" PRIx64 "%016" PRIx64,
                      transaction_id.a(), transaction_id.b(),
                      transaction_id.c());
}

ostream& operator<<(ostream& os, const TransactionId& transaction_id) {
  os << TransactionIdToString(transaction_id);
  return os;
}

}  // namespace engine
}  // namespace floating_temple
