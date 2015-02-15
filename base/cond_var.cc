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

#include "base/cond_var.h"

#include <pthread.h>

#include <cstddef>
#include <ctime>

#include <gflags/gflags.h>

#include <glog/logging.h>

#include "base/integral_types.h"
#include "base/logging.h"
#include "base/mutex.h"
#include "base/time_util.h"

using google::posix_strerror_r;
using std::time_t;

DEFINE_int32(cond_var_timeout_sec_for_debugging, -1,
             "If this flag is set, the process will crash if it needs to wait "
             "more than the specified number of seconds for a condition "
             "variable to be signaled. (For debugging only.)");

namespace floating_temple {

CondVar::CondVar() {
  CHECK_PTHREAD_ERR(pthread_cond_init(&cnd_, NULL));
}

CondVar::~CondVar() {
  CHECK_PTHREAD_ERR(pthread_cond_destroy(&cnd_));
}

void CondVar::Wait(Mutex* mu) const {
  if (FLAGS_cond_var_timeout_sec_for_debugging >= 0) {
    const int64 deadline_usec =
        GetCurrentTimeUsec() +
        static_cast<int64>(FLAGS_cond_var_timeout_sec_for_debugging) * 1000000;
    timespec deadline;
    deadline.tv_sec = static_cast<time_t>(deadline_usec / 1000000);
    deadline.tv_nsec = static_cast<long>(deadline_usec % 1000000 * 1000);

    CHECK(this->TimedWait(mu, &deadline));
  } else {
    this->WaitPatiently(mu);
  }
}

void CondVar::WaitPatiently(Mutex* mu) const {
  CHECK(mu != NULL);
  CHECK_PTHREAD_ERR(pthread_cond_wait(&cnd_, &mu->mtx_));
}

bool CondVar::TimedWait(Mutex* mu, const timespec* deadline) const {
  CHECK(mu != NULL);

  const int error_code = pthread_cond_timedwait(&cnd_, &mu->mtx_, deadline);

  if (error_code != 0) {
    if (error_code != ETIMEDOUT) {
      // Different versions of glibc provide implementations of strerror_r that
      // behave differently and have different return types. The Google logging
      // library's posix_strerror_r function abstracts away these compatibility
      // issues.

      char buf[100];
      posix_strerror_r(error_code, buf, sizeof buf);
      LOG(FATAL) << "pthread_cond_timedwait: " << buf << " [" << error_code
                 << "]";
    }

    return false;
  }

  return true;
}

}  // namespace floating_temple
