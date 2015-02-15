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

#ifndef BASE_NOTIFICATION_H_
#define BASE_NOTIFICATION_H_

#include "base/cond_var.h"
#include "base/macros.h"
#include "base/mutex.h"

namespace floating_temple {

// A synchronization object that allows threads to wait for an event to occur in
// a different thread. Currently, there is no way to reset the Notification
// object once it's been signaled.
class Notification {
 public:
  Notification();
  ~Notification();

  // Returns true if Notify has been called.
  bool notified() const;

  // Blocks until Notify is called.
  void Wait() const;

  // timeout_ms must be non-negative.
  //
  // Returns true if Notify was called before the timeout expired. Returns false
  // if the wait timed out. Crashes if the wait was interrupted by a signal.
  bool WaitWithTimeout(int timeout_ms) const;

  // Wakes up all waiting threads, and ensures that future calls to Wait or
  // WaitWithTimeout will return immediately.
  void Notify();

 private:
  bool notified_;
  mutable CondVar cond_;
  mutable Mutex mu_;

  DISALLOW_COPY_AND_ASSIGN(Notification);
};

}  // namespace floating_temple

#endif  // BASE_NOTIFICATION_H_
