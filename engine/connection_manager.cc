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

#include "engine/connection_manager.h"

#include <string>
#include <unordered_map>
#include <vector>

#include "base/cond_var.h"
#include "base/intrusive_ptr.h"
#include "base/logging.h"
#include "base/mutex.h"
#include "base/mutex_lock.h"
#include "engine/canonical_peer.h"
#include "engine/connection_handler.h"
#include "engine/peer_connection.h"
#include "engine/peer_id.h"
#include "engine/proto/peer.pb.h"
#include "protocol_server/protocol_server.h"
#include "util/state_variable.h"

using std::string;
using std::unordered_map;
using std::vector;

namespace floating_temple {

class ProtocolConnection;

namespace engine {

ConnectionManager::ConnectionManager()
    : canonical_peer_map_(nullptr),
      local_peer_(nullptr),
      connection_handler_(nullptr),
      state_(NOT_STARTED) {
  state_.AddStateTransition(NOT_STARTED, STARTING);
  state_.AddStateTransition(STARTING, RUNNING);
  state_.AddStateTransition(RUNNING, STOPPING);
  state_.AddStateTransition(STOPPING, STOPPED);
}

ConnectionManager::~ConnectionManager() {
  state_.CheckState(NOT_STARTED | STOPPED);
}

void ConnectionManager::ConnectToRemotePeer(const CanonicalPeer* remote_peer) {
  CHECK(remote_peer != nullptr);

  const string& peer_id = remote_peer->peer_id();

  string address;
  int port = 0;
  CHECK(ParsePeerId(peer_id, &address, &port))
      << "Invalid peer id: " << peer_id;

  const intrusive_ptr<PeerConnection> peer_connection(
      new PeerConnection(this, canonical_peer_map_, nullptr, address, true));

  {
    MutexLock lock(&connections_mu_);
    CHECK(unnamed_connections_.emplace(peer_connection.get(),
                                       peer_connection).second);
  }

  ProtocolConnection* const connection = protocol_server_.OpenConnection(
      peer_connection.get(), address, port);
  LOG(INFO) << "Successfully connected to peer " << peer_id << " (peer "
            << "connection " << peer_connection.get() << ")";
  peer_connection->Init(connection);
}

void ConnectionManager::Start(CanonicalPeerMap* canonical_peer_map,
                              const string& interpreter_type,
                              const CanonicalPeer* local_peer,
                              ConnectionHandler* connection_handler,
                              int send_receive_thread_count) {
  CHECK(canonical_peer_map != nullptr);
  CHECK(!interpreter_type.empty());
  CHECK(local_peer != nullptr);
  CHECK(connection_handler != nullptr);

  ChangeState(STARTING);

  canonical_peer_map_ = canonical_peer_map;
  interpreter_type_ = interpreter_type;
  local_peer_ = local_peer;
  connection_handler_ = connection_handler;

  const string& local_peer_id = local_peer->peer_id();

  string local_address;
  int listen_port = 0;
  CHECK(ParsePeerId(local_peer_id, &local_address, &listen_port))
      << local_peer_id;

  protocol_server_.Start(this, local_address, listen_port,
                         send_receive_thread_count);

  ChangeState(RUNNING);
}

void ConnectionManager::Stop() {
  ChangeState(STOPPING);

  DrainAllConnections();
  protocol_server_.Stop();

  ChangeState(STOPPED);
}

void ConnectionManager::SendMessageToRemotePeer(
    const CanonicalPeer* canonical_peer, const PeerMessage& peer_message,
    SendMode send_mode) {
  CHECK(canonical_peer != nullptr);

  while (state_.MatchesStateMask(NOT_STARTED | STARTING | RUNNING)) {
    const intrusive_ptr<PeerConnection> peer_connection = GetConnectionToPeer(
        canonical_peer);

    if (peer_connection->SendMessage(peer_message, send_mode)) {
      return;
    }

    VLOG(1) << "The attempt to send a message to peer "
            << canonical_peer->peer_id() << " failed temporarily because the "
            << "connection was being drained. (peer connection "
            << peer_connection.get() << ")";
  }
}

void ConnectionManager::BroadcastMessage(const PeerMessage& peer_message,
                                         SendMode send_mode) {
  vector<intrusive_ptr<PeerConnection>> connections;
  GetAllOpenConnections(&connections);

  for (const intrusive_ptr<PeerConnection>& connection : connections) {
    connection->SendMessage(peer_message, send_mode);
  }
}

void ConnectionManager::ChangeState(unsigned new_state) {
  state_.ChangeState(new_state);
}

intrusive_ptr<PeerConnection> ConnectionManager::GetConnectionToPeer(
    const CanonicalPeer* canonical_peer) {
  CHECK(canonical_peer != nullptr);

  const string& peer_id = canonical_peer->peer_id();

  string address;
  int port = 0;
  CHECK(ParsePeerId(peer_id, &address, &port))
      << "Invalid peer id: " << peer_id;

  bool connection_is_new = false;
  const intrusive_ptr<PeerConnection> peer_connection =
      GetOrCreateNamedConnection(canonical_peer, address, &connection_is_new);

  CHECK(peer_connection.get() != nullptr);

  if (connection_is_new) {
    ProtocolConnection* const connection = protocol_server_.OpenConnection(
        peer_connection.get(), address, port);
    LOG(INFO) << "Successfully connected to peer " << peer_id << " (peer "
              << "connection " << peer_connection.get() << ")";
    peer_connection->Init(connection);
  }

  return peer_connection;
}

void ConnectionManager::GetAllOpenConnections(
    vector<intrusive_ptr<PeerConnection>>* peer_connections) {
  CHECK(peer_connections != nullptr);
  peer_connections->clear();

  MutexLock lock(&connections_mu_);

  peer_connections->reserve(
      named_connections_.size() + unnamed_connections_.size());

  for (const auto& connection_pair : named_connections_) {
    peer_connections->push_back(connection_pair.second);
  }
  for (const auto& connection_pair : unnamed_connections_) {
    peer_connections->push_back(connection_pair.second);
  }
}

void ConnectionManager::DrainAllConnections() {
  for (;;) {
    vector<intrusive_ptr<PeerConnection>> connections;
    GetAllOpenConnections(&connections);

    for (const intrusive_ptr<PeerConnection>& connection : connections) {
      connection->Drain();
    }

    {
      MutexLock lock(&connections_mu_);

      if (named_connections_.empty() && unnamed_connections_.empty()) {
        return;
      }

      connections_empty_cond_.Wait(&connections_mu_);
    }
  }
}

intrusive_ptr<PeerConnection> ConnectionManager::GetOrCreateNamedConnection(
    const CanonicalPeer* canonical_peer, const string& address,
    bool* connection_is_new) {
  CHECK(canonical_peer != nullptr);
  CHECK(connection_is_new != nullptr);

  MutexLock lock(&connections_mu_);

  intrusive_ptr<PeerConnection>& peer_connection_ptr =
      named_connections_[canonical_peer];

  if (peer_connection_ptr.get() == nullptr) {
    peer_connection_ptr.reset(new PeerConnection(this, canonical_peer_map_,
                                                 canonical_peer, address,
                                                 true));
    *connection_is_new = true;
  } else {
    *connection_is_new = false;
  }

  return peer_connection_ptr;
}

const CanonicalPeer* ConnectionManager::GetConnectionInitiator(
    const PeerConnection* peer_connection) const {
  CHECK(peer_connection != nullptr);

  if (peer_connection->locally_initiated()) {
    return local_peer_;
  } else {
    return peer_connection->remote_peer();
  }
}

ProtocolConnectionHandler<PeerMessage>*
ConnectionManager::NotifyConnectionReceived(ProtocolConnection* connection,
                                            const string& remote_address) {
  const intrusive_ptr<PeerConnection> peer_connection(
      new PeerConnection(this, canonical_peer_map_, nullptr, remote_address,
                         false));
  peer_connection->Init(connection);

  LOG(INFO) << "Received a connection from a remote peer at address "
            << remote_address << " (peer connection " << peer_connection.get()
            << ")";

  {
    MutexLock lock(&connections_mu_);
    unnamed_connections_.emplace(peer_connection.get(), peer_connection);
  }

  return peer_connection.get();
}

void ConnectionManager::NotifyConnectionClosed(
    ProtocolConnectionHandler<PeerMessage>* connection_handler) {
  CHECK(connection_handler != nullptr);

  PeerConnection* const peer_connection = static_cast<PeerConnection*>(
      connection_handler);

  const CanonicalPeer* const remote_peer = peer_connection->remote_peer();

  if (remote_peer == nullptr) {
    VLOG(1) << "The connection to the peer at address "
            << peer_connection->remote_address() << " has been closed. (peer "
            << "connection " << peer_connection << ")";
  } else {
    VLOG(1) << "The connection to the peer " << remote_peer->peer_id()
            << " has been closed. (peer connection " << peer_connection << ")";
  }

  intrusive_ptr<PeerConnection> peer_connection_ptr;
  {
    MutexLock lock(&connections_mu_);

    unordered_map<const CanonicalPeer*, intrusive_ptr<PeerConnection>>::iterator
        named_connection_it;
    if (remote_peer == nullptr) {
      named_connection_it = named_connections_.end();
    } else {
      named_connection_it = named_connections_.find(remote_peer);
    }

    if (named_connection_it == named_connections_.end() ||
        named_connection_it->second.get() != peer_connection) {
      const unordered_map<PeerConnection*, intrusive_ptr<PeerConnection>>::
          iterator unnamed_connection_it = unnamed_connections_.find(
          peer_connection);
      CHECK(unnamed_connection_it != unnamed_connections_.end());

      peer_connection_ptr = unnamed_connection_it->second;
      unnamed_connections_.erase(unnamed_connection_it);
    } else {
      CHECK(unnamed_connections_.find(peer_connection) ==
            unnamed_connections_.end());

      peer_connection_ptr = named_connection_it->second;
      named_connections_.erase(named_connection_it);
    }

    if (named_connections_.empty() && unnamed_connections_.empty()) {
      connections_empty_cond_.Broadcast();
    }
  }

  peer_connection_ptr->Close();
}

const string& ConnectionManager::interpreter_type() const {
  CHECK(!interpreter_type_.empty());
  return interpreter_type_;
}

const CanonicalPeer* ConnectionManager::local_peer() const {
  CHECK(local_peer_ != nullptr);
  return local_peer_;
}

void ConnectionManager::NotifyRemotePeerKnown(
    PeerConnection* peer_connection, const CanonicalPeer* remote_peer) {
  CHECK(local_peer_ != nullptr);
  CHECK(connection_handler_ != nullptr);
  CHECK(peer_connection != nullptr);
  CHECK(remote_peer != nullptr);
  CHECK_NE(local_peer_, remote_peer);

  const string& remote_peer_id = remote_peer->peer_id();

  LOG(INFO) << "The remote peer at address "
            << peer_connection->remote_address() << " has identified itself as "
            << remote_peer_id << " (peer connection " << peer_connection << ")";

  intrusive_ptr<PeerConnection> peer_connection_to_drain;
  {
    MutexLock lock(&connections_mu_);

    intrusive_ptr<PeerConnection>& named_connection =
        named_connections_[remote_peer];

    const unordered_map<PeerConnection*, intrusive_ptr<PeerConnection>>::
        iterator unnamed_connection_it = unnamed_connections_.find(
        peer_connection);

    if (named_connection.get() == peer_connection) {
      // The remote peer id was already known. Do nothing.

      CHECK(unnamed_connection_it == unnamed_connections_.end());
    } else {
      if (named_connection.get() == nullptr) {
        // The remote peer id was not known until now. Move the connection from
        // the set of unnamed connections to the map of named connections.

        CHECK(unnamed_connection_it != unnamed_connections_.end());

        named_connection = unnamed_connection_it->second;
        unnamed_connections_.erase(unnamed_connection_it);
      } else {
        // A separate connection to the same remote peer already exists. This
        // can happen if two peers initiate connections to each other at about
        // the same time. Keep the connection that was initiated by the peer
        // whose peer id comes first in dictionary sort order, and drain the
        // other connection.

        CHECK(unnamed_connection_it != unnamed_connections_.end());

        const CanonicalPeer* const existing_connection_initiator =
            GetConnectionInitiator(named_connection.get());
        const CanonicalPeer* const new_connection_initiator =
            GetConnectionInitiator(peer_connection);

        if (existing_connection_initiator->peer_id() >
            new_connection_initiator->peer_id()) {
          VLOG(1) << "This peer has two connections open to peer "
                  << remote_peer_id << ". Draining the existing connection "
                  << "(peer connection " << named_connection.get() << ").";

          peer_connection_to_drain = named_connection;

          CHECK(unnamed_connections_.emplace(peer_connection_to_drain.get(),
                                             peer_connection_to_drain).second);

          named_connection = unnamed_connection_it->second;
          unnamed_connections_.erase(unnamed_connection_it);
        } else {
          VLOG(1) << "This peer has two connections open to peer "
                  << remote_peer_id << ". Draining the new connection (peer "
                  << "connection " << peer_connection << ").";

          peer_connection_to_drain = unnamed_connection_it->second;
        }
      }
    }
  }

  if (peer_connection_to_drain.get() != nullptr) {
    peer_connection_to_drain->Drain();
  }

  // TODO(dss): Only notify the TransactionStore class if a connection to the
  // remote peer didn't already exist.
  connection_handler_->NotifyNewConnection(remote_peer);
}

void ConnectionManager::HandleMessageFromRemotePeer(
    const CanonicalPeer* remote_peer, const PeerMessage& peer_message) {
  CHECK(connection_handler_ != nullptr);
  connection_handler_->HandleMessageFromRemotePeer(remote_peer, peer_message);
}

}  // namespace engine
}  // namespace floating_temple
