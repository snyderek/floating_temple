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

#include "engine/transaction_id_generator.h"

#include <ctime>

#include "base/integral_types.h"
#include "base/logging.h"
#include "base/mutex_lock.h"
#include "engine/proto/transaction_id.pb.h"
#include "engine/proto/uuid.pb.h"
#include "engine/uuid_util.h"

namespace floating_temple {
namespace engine {

TransactionIdGenerator::TransactionIdGenerator()
    : last_time_value_(0) {
  GenerateUuid(&uuid_);
}

void TransactionIdGenerator::Generate(TransactionId* transaction_id) {
  CHECK(transaction_id != nullptr);

  timespec ts;
  CHECK_ERR(clock_gettime(CLOCK_REALTIME, &ts));

  uint64 time_value = static_cast<uint64>(ts.tv_sec) * 1000000000 +
                      static_cast<uint64>(ts.tv_nsec);

  {
    MutexLock lock(&mu_);

    if (time_value <= last_time_value_) {
      time_value = last_time_value_ + 1;
    }
    last_time_value_ = time_value;
  }

  transaction_id->Clear();
  transaction_id->set_a(time_value);
  transaction_id->set_b(uuid_.high_word());
  transaction_id->set_c(uuid_.low_word());
}

}  // namespace engine
}  // namespace floating_temple
