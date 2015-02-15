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

#ifndef PEER_PEER_CONNECTION_H_
#define PEER_PEER_CONNECTION_H_

#include <pthread.h>

#include <string>

#include "base/cond_var.h"
#include "base/macros.h"
#include "base/mutex.h"
#include "base/scoped_ptr.h"
#include "peer/peer_message_sender.h"
#include "protocol_server/protocol_connection_handler.h"
#include "util/quota_queue.h"

namespace floating_temple {

class ProtocolConnection;
class StateVariableInternalInterface;

namespace peer {

class CanonicalPeer;
class CanonicalPeerMap;
class ConnectionManagerInterfaceForPeerConnection;
class GoodbyeMessage;
class HelloMessage;
class PeerMessage;

class PeerConnection : public ProtocolConnectionHandler<PeerMessage> {
 public:
  PeerConnection(
      ConnectionManagerInterfaceForPeerConnection* connection_manager,
      CanonicalPeerMap* canonical_peer_map,
      const CanonicalPeer* remote_peer,
      const std::string& remote_address,
      bool locally_initiated);
  virtual ~PeerConnection();

  void Init(ProtocolConnection* connection);

  const CanonicalPeer* remote_peer() const { return PrivateGetRemotePeer(); }
  const std::string& remote_address() const { return remote_address_; }
  bool locally_initiated() const { return locally_initiated_; }

  bool SendMessage(const PeerMessage& peer_message,
                   PeerMessageSender::SendMode send_mode);

  void Drain();
  void Close();

  void IncrementRefCount();
  bool DecrementRefCount();

  virtual bool GetNextOutputMessage(PeerMessage* message);
  virtual void NotifyMessageReceived(const PeerMessage& message);

 private:
  enum ConnectionState {
    CONNECTION_OPEN,
    CONNECTION_CLOSED
  };

  enum ReceiveState {
    NO_MESSAGE_RECEIVED,
    HELLO_RECEIVED,
    GOODBYE_RECEIVED
  };

  enum SendState {
    NO_MESSAGE_SENT,
    HELLO_SENT,
    GOODBYE_SENT
  };

  enum DrainState {
    NO_DRAIN_REQUESTED,
    DRAIN_REQUESTED
  };

  ProtocolConnection* PrivateGetProtocolConnection() const;
  const CanonicalPeer* PrivateGetRemotePeer() const;

  bool GetNextOutputMessageHelper(PeerMessage* message);

  void SetRemotePeer(const CanonicalPeer* new_remote_peer);

  void HandleHelloMessage(const HelloMessage& hello_message);
  void HandleGoodbyeMessage(const GoodbyeMessage& goodbye_message);
  void HandleRegularMessage(const PeerMessage& peer_message);

  std::string GetRemotePeerIdForLogging() const;

  ConnectionManagerInterfaceForPeerConnection* const connection_manager_;
  // TODO(dss): This class should not need to store a pointer to the
  // CanonicalPeerMap instance.
  CanonicalPeerMap* const canonical_peer_map_;
  const std::string remote_address_;
  const bool locally_initiated_;

  scoped_ptr<ProtocolConnection> protocol_connection_;
  mutable CondVar protocol_connection_set_cond_;
  mutable Mutex protocol_connection_mu_;

  const CanonicalPeer* remote_peer_;
  mutable Mutex remote_peer_mu_;

  ConnectionState connection_state_;
  ReceiveState receive_state_;
  SendState send_state_;
  DrainState drain_state_;
  mutable Mutex state_mu_;

  // TODO(dss): The message queue should be separate from the connection object,
  // so that no messages will be dropped if the connection is closed and needs
  // to be reestablished.
  QuotaQueue<PeerMessage*> output_messages_;

  int ref_count_;
  mutable Mutex ref_count_mu_;

  DISALLOW_COPY_AND_ASSIGN(PeerConnection);
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_PEER_CONNECTION_H_
