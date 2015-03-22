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

#ifndef PROTOCOL_SERVER_FORMAT_PROTOCOL_MESSAGE_H_
#define PROTOCOL_SERVER_FORMAT_PROTOCOL_MESSAGE_H_

#include <string>

#include "base/integral_types.h"
#include "base/logging.h"
#include "protocol_server/varint.h"

namespace floating_temple {

template<class Message>
void FormatProtocolMessage(const Message& message, std::string* output) {
  CHECK(output != nullptr);

  output->clear();

  const int message_length = message.ByteSize();
  CHECK_GE(message_length, 0);

  output->reserve(
      static_cast<std::string::size_type>(kMaxVarintLength + message_length));

  char buffer[kMaxVarintLength];
  const int varint_length = FormatVarint(static_cast<uint64>(message_length),
                                         buffer, kMaxVarintLength);
  CHECK_LE(varint_length, kMaxVarintLength);

  output->append(buffer, static_cast<std::string::size_type>(varint_length));
  CHECK(message.AppendToString(output));
}

}  // namespace floating_temple

#endif  // PROTOCOL_SERVER_FORMAT_PROTOCOL_MESSAGE_H_
