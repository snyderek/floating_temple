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

#ifndef ENGINE_MOCK_CONNECTION_HANDLER_H_
#define ENGINE_MOCK_CONNECTION_HANDLER_H_

#include "base/macros.h"
#include "engine/connection_handler.h"
#include "third_party/gmock-1.7.0/include/gmock/gmock.h"

namespace floating_temple {
namespace engine {

class MockConnectionHandler : public ConnectionHandler {
 public:
  MockConnectionHandler() {}

  MOCK_METHOD1(NotifyNewConnection, void(const CanonicalPeer* remote_peer));
  MOCK_METHOD2(HandleMessageFromRemotePeer,
               void(const CanonicalPeer* remote_peer,
                    const PeerMessage& peer_message));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockConnectionHandler);
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_MOCK_CONNECTION_HANDLER_H_
