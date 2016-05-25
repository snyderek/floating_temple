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

#ifndef ENGINE_CONNECTION_MANAGER_H_
#define ENGINE_CONNECTION_MANAGER_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "base/cond_var.h"
#include "base/intrusive_ptr.h"
#include "base/macros.h"
#include "base/mutex.h"
#include "engine/connection_manager_interface_for_peer_connection.h"
#include "engine/peer_message_sender.h"
#include "engine/proto/peer.pb.h"
#include "protocol_server/protocol_server.h"
#include "protocol_server/protocol_server_handler.h"
#include "util/state_variable.h"

namespace floating_temple {

class ProtocolConnection;

namespace engine {

class CanonicalPeer;
class CanonicalPeerMap;
class ConnectionHandler;
class PeerConnection;

// TODO(dss): Rename this class to PeerConnectionManager, since it manages
// instances of type PeerConnection.
class ConnectionManager : public PeerMessageSender,
                          private ProtocolServerHandler<PeerMessage>,
                          private ConnectionManagerInterfaceForPeerConnection {
 public:
  ConnectionManager();
  ~ConnectionManager() override;

  void ConnectToRemotePeer(const CanonicalPeer* remote_peer);

  void Start(CanonicalPeerMap* canonical_peer_map,
             const std::string& interpreter_type,
             const CanonicalPeer* local_peer,
             ConnectionHandler* connection_handler,
             int send_receive_thread_count);
  void Stop();

  void SendMessageToRemotePeer(const CanonicalPeer* canonical_peer,
                               const PeerMessage& peer_message,
                               SendMode send_mode) override;
  void BroadcastMessage(const PeerMessage& peer_message,
                        SendMode send_mode) override;

 private:
  enum {
    NOT_STARTED = 0x1,
    STARTING = 0x2,
    RUNNING = 0x4,
    STOPPING = 0x8,
    STOPPED = 0x10
  };

  void ChangeState(unsigned new_state);

  intrusive_ptr<PeerConnection> GetConnectionToPeer(
      const CanonicalPeer* canonical_peer);
  ProtocolConnection* ConnectToPeer(PeerConnection* connection_handler,
                                    const std::string& peer_id,
                                    const std::string& address,
                                    int port);
  void GetAllOpenConnections(
      std::vector<intrusive_ptr<PeerConnection>>* peer_connections);
  void DrainAllConnections();

  intrusive_ptr<PeerConnection> GetOrCreateNamedConnection(
      const CanonicalPeer* canonical_peer, const std::string& address,
      bool* connection_is_new);

  const CanonicalPeer* GetConnectionInitiator(
      const PeerConnection* peer_connection) const;

  ProtocolConnectionHandler<PeerMessage>* NotifyConnectionReceived(
      ProtocolConnection* connection,
      const std::string& remote_address) override;
  void NotifyConnectionClosed(
      ProtocolConnectionHandler<PeerMessage>* connection_handler) override;

  const std::string& interpreter_type() const override;
  const CanonicalPeer* local_peer() const override;
  void NotifyRemotePeerKnown(PeerConnection* peer_connection,
                             const CanonicalPeer* remote_peer) override;
  void HandleMessageFromRemotePeer(const CanonicalPeer* remote_peer,
                                   const PeerMessage& peer_message) override;

  CanonicalPeerMap* canonical_peer_map_;
  std::string interpreter_type_;
  const CanonicalPeer* local_peer_;
  ConnectionHandler* connection_handler_;
  ProtocolServer<PeerMessage> protocol_server_;

  StateVariable state_;

  std::unordered_map<const CanonicalPeer*, intrusive_ptr<PeerConnection>>
      named_connections_;
  std::unordered_map<PeerConnection*, intrusive_ptr<PeerConnection>>
      unnamed_connections_;
  mutable CondVar connections_empty_cond_;
  mutable Mutex connections_mu_;

  DISALLOW_COPY_AND_ASSIGN(ConnectionManager);
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_CONNECTION_MANAGER_H_
