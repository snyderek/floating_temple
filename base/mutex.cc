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

#include "base/mutex.h"

#include <pthread.h>

#include <ctime>

#include <gflags/gflags.h>

#include "base/logging.h"

using std::time_t;

DEFINE_int32(mutex_timeout_sec_for_debugging, -1,
             "If this flag is set, the process will crash if it needs to wait "
             "more than the specified number of seconds for a mutex to be "
             "unlocked. (For debugging only.)");

namespace floating_temple {

Mutex::Mutex() {
  pthread_mutexattr_t mutex_attributes;
  CHECK_PTHREAD_ERR(pthread_mutexattr_init(&mutex_attributes));
  CHECK_PTHREAD_ERR(pthread_mutexattr_settype(&mutex_attributes,
                                              PTHREAD_MUTEX_ERRORCHECK));

  CHECK_PTHREAD_ERR(pthread_mutex_init(&mtx_, &mutex_attributes));

  CHECK_PTHREAD_ERR(pthread_mutexattr_destroy(&mutex_attributes));
}

Mutex::~Mutex() {
  CHECK_PTHREAD_ERR(pthread_mutex_destroy(&mtx_));
}

void Mutex::Lock() {
  if (FLAGS_mutex_timeout_sec_for_debugging >= 0) {
    timespec deadline;
    CHECK_ERR(clock_gettime(CLOCK_REALTIME, &deadline));
    deadline.tv_sec +=
        static_cast<time_t>(FLAGS_mutex_timeout_sec_for_debugging);

    CHECK_PTHREAD_ERR(pthread_mutex_timedlock(&mtx_, &deadline));
  } else {
    CHECK_PTHREAD_ERR(pthread_mutex_lock(&mtx_));
  }
}

}  // namespace floating_temple
