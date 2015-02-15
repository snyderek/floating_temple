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

#include "util/event_fd.h"

#include <unistd.h>

#include <cerrno>

#include "base/integral_types.h"
#include "base/logging.h"

namespace floating_temple {

void SignalEventFd(int event_fd) {
  VLOG(1) << "Signaling event FD " << event_fd;
  const uint64 increment = 1;
  CHECK_ERR(write(event_fd, &increment, sizeof increment));
}

void ClearEventFd(int event_fd) {
  uint64 counter = 0;
  const ssize_t byte_count = read(event_fd, &counter, sizeof counter);

  if (byte_count == -1) {
    PLOG_IF(FATAL, errno != EAGAIN && errno != EWOULDBLOCK) << "read";
  } else {
    CHECK_EQ(byte_count, static_cast<ssize_t>(sizeof counter));
  }
}

}  // namespace floating_temple
