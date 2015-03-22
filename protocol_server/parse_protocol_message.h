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

#ifndef PROTOCOL_SERVER_PARSE_PROTOCOL_MESSAGE_H_
#define PROTOCOL_SERVER_PARSE_PROTOCOL_MESSAGE_H_

#include <string>

#include "base/logging.h"

namespace floating_temple {

// The following functions, if successful, return the number of characters that
// were parsed. If the input is incomplete, they return -1 and leave the output
// parameter unchanged.

int ParseMessageLength(const char* input_buffer, int buffer_size,
                       int* message_length);

template<class Message>
int ParseProtocolMessage(const char* input_buffer, int buffer_size,
                         Message* message) {
  CHECK(input_buffer != nullptr);
  CHECK(message != nullptr);

  int message_length = 0;
  const int varint_length = ParseMessageLength(input_buffer, buffer_size,
                                               &message_length);
  CHECK_GE(message_length, 0);

  if (varint_length < 0 || varint_length + message_length > buffer_size) {
    return -1;
  }

  std::string encoded_message;
  encoded_message.assign(input_buffer + varint_length,
                         static_cast<std::string::size_type>(message_length));

  // TODO(dss): Fail gracefully if the remote peer sends an improperly encoded
  // protocol message.
  CHECK(message->ParseFromString(encoded_message));

  return varint_length + message_length;
}

}  // namespace floating_temple

#endif  // PROTOCOL_SERVER_PARSE_PROTOCOL_MESSAGE_H_
