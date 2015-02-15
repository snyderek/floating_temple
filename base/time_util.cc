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

#include "base/time_util.h"

#include <sys/time.h>

#include <cstddef>

#include "base/integral_types.h"
#include "base/logging.h"

namespace floating_temple {

int64 GetCurrentTimeUsec() {
  timeval tv;
  CHECK_EQ(gettimeofday(&tv, NULL), 0);
  return static_cast<int64>(tv.tv_sec) * 1000000 +
         static_cast<int64>(tv.tv_usec);
}

}  // namespace floating_temple