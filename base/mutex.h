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

#ifndef BASE_MUTEX_H_
#define BASE_MUTEX_H_

#include <pthread.h>

#include "base/logging.h"
#include "base/macros.h"

namespace floating_temple {

class CondVar;

// Wrapper class for a Pthreads mutex. The mutex type is set to
// PTHREAD_MUTEX_ERRORCHECK. This class may support other mutex types in the
// future, but for now the error checking makes testing easier.
class Mutex {
 public:
  Mutex();
  ~Mutex();

  void Lock();
  void Unlock();

 private:
  pthread_mutex_t mtx_;

  friend class CondVar;

  DISALLOW_COPY_AND_ASSIGN(Mutex);
};

inline void Mutex::Unlock() {
  CHECK_PTHREAD_ERR(pthread_mutex_unlock(&mtx_));
}

}  // namespace floating_temple

#endif  // BASE_MUTEX_H_
