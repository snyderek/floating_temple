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

#include "peer/min_version_map.h"

#include "peer/transaction_id_util.h"

namespace floating_temple {
namespace peer {

bool TransactionIdLessThanFunction::operator()(const TransactionId& a,
                                               const TransactionId& b) const {
  return CompareTransactionIds(a, b) < 0;
}

}  // namespace peer
}  // namespace floating_temple
