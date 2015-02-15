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

#include "protocol_server/parse_protocol_message.h"

#include <cstddef>

#include "base/integral_types.h"
#include "base/logging.h"
#include "protocol_server/varint.h"

namespace floating_temple {

int ParseMessageLength(const char* input_buffer, int buffer_size,
                       int* message_length) {
  CHECK(message_length != NULL);

  uint64 varint = 0u;
  const int varint_length = ParseVarint(input_buffer, buffer_size, &varint);

  if (varint_length < 0) {
    return -1;
  }

  const int message_length_temp = static_cast<int>(varint);
  // TODO(dss): Fail gracefully if the remote peer sends a variable-length
  // integer that's too large.
  CHECK_GE(message_length_temp, 0) << "Arithmetic overflow";

  *message_length = message_length_temp;
  return varint_length;
}

}  // namespace floating_temple
