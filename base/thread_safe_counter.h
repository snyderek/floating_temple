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

#ifndef BASE_THREAD_SAFE_COUNTER_H_
#define BASE_THREAD_SAFE_COUNTER_H_

#include "base/cond_var.h"
#include "base/macros.h"
#include "base/mutex.h"

namespace floating_temple {

class ThreadSafeCounter {
 public:
  // initial_value must be non-negative.
  explicit ThreadSafeCounter(int initial_value);

  void Decrement();

  // Blocks until the counter is zero.
  void WaitForZero() const;

  // timeout_ms must be non-negative.
  //
  // Returns true if the counter became zero before the timeout expired (or if
  // the counter was already zero). Returns false if the wait timed out. Crashes
  // if the wait was interrupted by a signal.
  bool WaitForZeroWithTimeout(int timeout_ms) const;

 private:
  int value_;
  mutable CondVar cond_;
  mutable Mutex mu_;

  DISALLOW_COPY_AND_ASSIGN(ThreadSafeCounter);
};

}  // namespace floating_temple

#endif  // BASE_THREAD_SAFE_COUNTER_H_
