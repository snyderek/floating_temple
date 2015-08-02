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

#ifndef BASE_COND_VAR_H_
#define BASE_COND_VAR_H_

#include <pthread.h>

#include "base/logging.h"
#include "base/macros.h"

namespace floating_temple {

class Mutex;

// Wrapper class for a Pthreads condition variable, with default attributes.
class CondVar {
 public:
  CondVar();
  ~CondVar();

  // Waits for the condition variable to be signaled. This is the default wait
  // operation.
  //
  // 'mu' must not be NULL.
  void Wait(Mutex* mu) const;

  // Waits for the condition variable to be signaled. The difference between
  // this method and Wait is that, during debugging, this method ignores the
  // flag-configured timeout value and waits indefinitely. Otherwise, the two
  // methods are functionally equivalent.
  //
  // 'mu' must not be NULL.
  void WaitPatiently(Mutex* mu) const;

  // 'mu' and 'deadline' must both be non-NULL.
  //
  // Returns true if the condition variable was signaled before the deadline.
  // Returns false if the wait timed out. Crashes if the wait is interrupted by
  // a signal.
  bool TimedWait(Mutex* mu, const timespec* deadline) const;

  // Wakes one thread that's waiting on the condition variable.
  void Signal();
  // Wakes all threads that are waiting on the condition variable.
  void Broadcast();

 private:
  mutable pthread_cond_t cnd_;

  DISALLOW_COPY_AND_ASSIGN(CondVar);
};

inline void CondVar::Signal() {
  CHECK_PTHREAD_ERR(pthread_cond_signal(&cnd_));
}

inline void CondVar::Broadcast() {
  CHECK_PTHREAD_ERR(pthread_cond_broadcast(&cnd_));
}

}  // namespace floating_temple

#endif  // BASE_COND_VAR_H_
