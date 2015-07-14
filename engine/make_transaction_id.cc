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

#include "engine/make_transaction_id.h"

#include "base/integral_types.h"
#include "engine/proto/transaction_id.pb.h"

namespace floating_temple {
namespace peer {

TransactionId MakeTransactionId(uint64 a, uint64 b, uint64 c) {
  TransactionId transaction_id;
  transaction_id.set_a(a);
  transaction_id.set_b(b);
  transaction_id.set_c(c);

  return transaction_id;
}

}  // namespace peer
}  // namespace floating_temple
