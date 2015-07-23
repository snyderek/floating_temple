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

#ifndef ENGINE_CONNECTION_MANAGER_INTERFACE_FOR_PEER_CONNECTION_H_
#define ENGINE_CONNECTION_MANAGER_INTERFACE_FOR_PEER_CONNECTION_H_

#include <string>

namespace floating_temple {
namespace engine {

class CanonicalPeer;
class PeerConnection;
class PeerMessage;

class ConnectionManagerInterfaceForPeerConnection {
 public:
  virtual ~ConnectionManagerInterfaceForPeerConnection() {}

  virtual const std::string& interpreter_type() const = 0;
  virtual const CanonicalPeer* local_peer() const = 0;

  // TODO(dss): The remote_peer parameter is unnecessary because the callee can
  // call peer_connection->remote_peer().
  virtual void NotifyRemotePeerKnown(PeerConnection* peer_connection,
                                     const CanonicalPeer* remote_peer) = 0;
  virtual void HandleMessageFromRemotePeer(const CanonicalPeer* remote_peer,
                                           const PeerMessage& peer_message) = 0;
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_CONNECTION_MANAGER_INTERFACE_FOR_PEER_CONNECTION_H_
