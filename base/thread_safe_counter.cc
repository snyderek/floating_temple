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

#include "base/thread_safe_counter.h"

#include <ctime>

#include "base/cond_var.h"
#include "base/integral_types.h"
#include "base/logging.h"
#include "base/mutex.h"
#include "base/mutex_lock.h"
#include "base/time_util.h"

using std::time_t;

namespace floating_temple {

ThreadSafeCounter::ThreadSafeCounter(int initial_value)
    : value_(initial_value) {
  CHECK_GE(initial_value, 0);
}

void ThreadSafeCounter::Decrement() {
  MutexLock lock(&mu_);

  CHECK_GT(value_, 0);
  --value_;

  if (value_ == 0) {
    cond_.Broadcast();
  }
}

void ThreadSafeCounter::WaitForZero() const {
  MutexLock lock(&mu_);

  while (value_ > 0) {
    cond_.Wait(&mu_);
  }

  CHECK_EQ(value_, 0);
}

bool ThreadSafeCounter::WaitForZeroWithTimeout(int timeout_ms) const {
  CHECK_GE(timeout_ms, 0);

  const int64 deadline_usec = GetCurrentTimeUsec() + timeout_ms * 1000;
  timespec deadline;
  deadline.tv_sec = static_cast<time_t>(deadline_usec / 1000000);
  deadline.tv_nsec = static_cast<long>(deadline_usec % 1000000 * 1000);

  MutexLock lock(&mu_);

  if (timeout_ms > 0) {
    while (value_ > 0 && GetCurrentTimeUsec() <= deadline_usec) {
      cond_.TimedWait(&mu_, &deadline);
    }
  }

  CHECK_GE(value_, 0);

  return value_ == 0;
}

}  // namespace floating_temple
