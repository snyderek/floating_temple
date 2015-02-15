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

#ifndef PROTOCOL_SERVER_PROTOCOL_SERVER_HANDLER_H_
#define PROTOCOL_SERVER_PROTOCOL_SERVER_HANDLER_H_

#include <string>

namespace floating_temple {

class ProtocolConnection;
template<class Message> class ProtocolConnectionHandler;

template<class Message>
class ProtocolServerHandler {
 public:
  virtual ~ProtocolServerHandler() {}

  // The callee must take ownership of *connection. The caller does not take
  // ownership of the returned ProtocolConnectionHandler instance.
  virtual ProtocolConnectionHandler<Message>* NotifyConnectionReceived(
      ProtocolConnection* connection, const std::string& remote_address) = 0;

  virtual void NotifyConnectionClosed(
      ProtocolConnectionHandler<Message>* connection_handler) = 0;
};

}  // namespace floating_temple

#endif  // PROTOCOL_SERVER_PROTOCOL_SERVER_HANDLER_H_