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

#include "protocol_server/buffer_util.h"

#include <cstddef>

#include "base/logging.h"

namespace floating_temple {

const char* FindCharInRange(const char* start, const char* end, char c) {
  CHECK(start != NULL);
  CHECK(end != NULL);
  CHECK_LE(start, end);

  const char* p = start;
  while (p < end && *p != c) {
    ++p;
  }
  return p;
}

}  // namespace floating_temple