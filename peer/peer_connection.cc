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

#include "peer/peer_connection.h"

#include <cstddef>
#include <string>

#include "base/cond_var.h"
#include "base/logging.h"
#include "base/mutex.h"
#include "base/mutex_lock.h"
#include "base/scoped_ptr.h"
#include "base/string_printf.h"
#include "peer/canonical_peer.h"
#include "peer/canonical_peer_map.h"
#include "peer/connection_manager_interface_for_peer_connection.h"
#include "peer/get_peer_message_type.h"
#include "peer/peer_message_sender.h"
#include "peer/proto/peer.pb.h"
#include "protocol_server/protocol_connection.h"
#include "util/quota_queue.h"

using std::string;

namespace floating_temple {
namespace peer {
namespace {

void CreateHelloMessage(
    ConnectionManagerInterfaceForPeerConnection* connection_manager,
    PeerMessage* peer_message) {
  CHECK(connection_manager != NULL);
  CHECK(peer_message != NULL);

  peer_message->Clear();

  HelloMessage* const hello_message = peer_message->mutable_hello_message();
  hello_message->set_peer_id(connection_manager->local_peer()->peer_id());
  hello_message->set_interpreter_type(connection_manager->interpreter_type());
}

void CreateGoodbyeMessage(PeerMessage* peer_message) {
  CHECK(peer_message != NULL);

  peer_message->Clear();
  peer_message->mutable_goodbye_message();
}

}  // namespace

PeerConnection::PeerConnection(
    ConnectionManagerInterfaceForPeerConnection* connection_manager,
    CanonicalPeerMap* canonical_peer_map,
    const CanonicalPeer* remote_peer,
    const string& remote_address,
    bool locally_initiated)
    : connection_manager_(CHECK_NOTNULL(connection_manager)),
      canonical_peer_map_(CHECK_NOTNULL(canonical_peer_map)),
      remote_address_(remote_address),
      locally_initiated_(locally_initiated),
      remote_peer_(remote_peer),
      connection_state_(CONNECTION_OPEN),
      receive_state_(NO_MESSAGE_RECEIVED),
      send_state_(NO_MESSAGE_SENT),
      drain_state_(NO_DRAIN_REQUESTED),
      output_messages_(-1),
      ref_count_(1) {
  CHECK(!remote_address.empty());

  // Add the service IDs in descending order so that the vector inside
  // QuotaQueue won't have to reallocate memory. (This is probably an
  // unnecessary optimization, but it's easy to do.)

  // The BLOCKING_MODE sub-queue is limited to a single message, so that calls
  // to PeerConnection::SendMessage(..., BLOCKING_MODE) will block if there's
  // already a message in that sub-queue.
  output_messages_.AddService(PeerMessageSender::BLOCKING_MODE, 1);

  // The NON_BLOCKING_MODE sub-queue is unlimited, so that calls to
  // PeerConnection::SendMessage(..., NON_BLOCKING_MODE) will never block.
  output_messages_.AddService(PeerMessageSender::NON_BLOCKING_MODE, -1);
}

PeerConnection::~PeerConnection() {
  PeerMessage* peer_message = NULL;
  int service_id = 0;

  while (output_messages_.Pop(&peer_message, &service_id, false)) {
    delete peer_message;
  }
}

void PeerConnection::Init(ProtocolConnection* connection) {
  CHECK(connection != NULL);

  {
    MutexLock lock(&protocol_connection_mu_);
    protocol_connection_.reset(connection);
    protocol_connection_set_cond_.Broadcast();
  }

  connection->NotifyMessageReadyToSend();
}

bool PeerConnection::SendMessage(const PeerMessage& peer_message,
                                 PeerMessageSender::SendMode send_mode) {
  PeerMessage* peer_message_copy = new PeerMessage();
  peer_message_copy->CopyFrom(peer_message);

  if (!output_messages_.Push(peer_message_copy, static_cast<int>(send_mode),
                             false)) {
    delete peer_message_copy;
    return false;
  }

  PrivateGetProtocolConnection()->NotifyMessageReadyToSend();

  return true;
}

void PeerConnection::Drain() {
  {
    MutexLock lock(&state_mu_);
    drain_state_ = DRAIN_REQUESTED;
  }

  output_messages_.Drain();

  ProtocolConnection* const protocol_connection =
      PrivateGetProtocolConnection();

  if (protocol_connection != NULL) {
    protocol_connection->NotifyMessageReadyToSend();
  }
}

void PeerConnection::Close() {
  MutexLock lock(&state_mu_);
  connection_state_ = CONNECTION_CLOSED;
}

void PeerConnection::IncrementRefCount() {
  MutexLock lock(&ref_count_mu_);
  ++ref_count_;
  CHECK_GE(ref_count_, 1);
}

bool PeerConnection::DecrementRefCount() {
  MutexLock lock(&ref_count_mu_);
  CHECK_GE(ref_count_, 1);
  --ref_count_;
  return ref_count_ == 0;
}

bool PeerConnection::GetNextOutputMessage(PeerMessage* message) {
  CHECK(message != NULL);

  if (!GetNextOutputMessageHelper(message)) {
    return false;
  }

  VLOG(1) << "Sending a "
          << PeerMessage::Type_Name(GetPeerMessageType(*message)) << " message "
          << "to peer " << GetRemotePeerIdForLogging() << " (peer connection "
          << this << ")";

  return true;
}

void PeerConnection::NotifyMessageReceived(const PeerMessage& message) {
  // TODO(dss): Fail gracefully if the remote peer violates the protocol.
  message.CheckInitialized();

  const PeerMessage::Type type = GetPeerMessageType(message);
  VLOG(1) << "Received a " << PeerMessage::Type_Name(type) << " message from "
          << "peer " << GetRemotePeerIdForLogging() << " (peer connection "
          << this << ")";

  switch (type) {
    case PeerMessage::HELLO:
      HandleHelloMessage(message.hello_message());
      break;

    case PeerMessage::GOODBYE:
      HandleGoodbyeMessage(message.goodbye_message());
      break;

    default:
      HandleRegularMessage(message);
  }
}

ProtocolConnection* PeerConnection::PrivateGetProtocolConnection() const {
  MutexLock lock(&protocol_connection_mu_);

  while (protocol_connection_.get() == NULL) {
    protocol_connection_set_cond_.Wait(&protocol_connection_mu_);
  }

  return protocol_connection_.get();
}

const CanonicalPeer* PeerConnection::PrivateGetRemotePeer() const {
  MutexLock lock(&remote_peer_mu_);
  return remote_peer_;
}

bool PeerConnection::GetNextOutputMessageHelper(PeerMessage* message) {
  CHECK(message != NULL);

  {
    MutexLock lock(&state_mu_);

    switch (send_state_) {
      case NO_MESSAGE_SENT:
        CreateHelloMessage(connection_manager_, message);
        send_state_ = HELLO_SENT;
        return true;

      case HELLO_SENT:
        break;

      default:
        return false;
    }
  }

  PeerMessage* regular_message = NULL;
  int service_id = 0;

  if (output_messages_.Pop(&regular_message, &service_id, false)) {
    CHECK(regular_message != NULL);

    message->Swap(regular_message);
    delete regular_message;

    return true;
  }

  bool close_connection = false;
  {
    MutexLock lock(&state_mu_);

    if (drain_state_ != DRAIN_REQUESTED) {
      return false;
    }

    CreateGoodbyeMessage(message);
    send_state_ = GOODBYE_SENT;

    if (receive_state_ == GOODBYE_RECEIVED) {
      close_connection = true;
    }
  }

  if (close_connection) {
    PrivateGetProtocolConnection()->Close();
  }

  return true;
}

void PeerConnection::SetRemotePeer(const CanonicalPeer* new_remote_peer) {
  CHECK(new_remote_peer != NULL);

  {
    MutexLock lock(&remote_peer_mu_);

    if (remote_peer_ == NULL) {
      remote_peer_ = new_remote_peer;
    } else {
      // TODO(dss): Fail gracefully if the remote peer sends a peer ID different
      // from the one that was expected.
      CHECK_EQ(remote_peer_, new_remote_peer);
    }
  }

  connection_manager_->NotifyRemotePeerKnown(this, new_remote_peer);
}

void PeerConnection::HandleHelloMessage(const HelloMessage& hello_message) {
  const string& new_remote_peer_id = hello_message.peer_id();
  CHECK_EQ(hello_message.interpreter_type(),
           connection_manager_->interpreter_type());

  {
    MutexLock lock(&state_mu_);

    // TODO(dss): Fail gracefully if the remote peer sends duplicate HELLO
    // messages, or sends a HELLO message after sending a GOODBYE message.
    CHECK_EQ(receive_state_, NO_MESSAGE_RECEIVED);
    receive_state_ = HELLO_RECEIVED;
  }

  const CanonicalPeer* const new_remote_peer =
      canonical_peer_map_->GetCanonicalPeer(new_remote_peer_id);

  SetRemotePeer(new_remote_peer);
}

void PeerConnection::HandleGoodbyeMessage(
    const GoodbyeMessage& goodbye_message) {
  bool close_connection = false;
  {
    MutexLock lock(&state_mu_);

    // TODO(dss): Fail gracefully if the remote peer sends duplicate GOODBYE
    // messages, or sends a GOODBYE message without first sending a HELLO
    // message.
    CHECK_EQ(receive_state_, HELLO_RECEIVED);

    receive_state_ = GOODBYE_RECEIVED;
    drain_state_ = DRAIN_REQUESTED;

    if (send_state_ == GOODBYE_SENT) {
      close_connection = true;
    }
  }

  output_messages_.Drain();

  if (close_connection) {
    PrivateGetProtocolConnection()->Close();
  }
}

void PeerConnection::HandleRegularMessage(const PeerMessage& peer_message) {
  {
    MutexLock lock(&state_mu_);

    // TODO(dss): Fail gracefully if the remote peer sends a regular message
    // before sending a HELLO message or after sending a GOODBYE message.
    CHECK_EQ(receive_state_, HELLO_RECEIVED);
  }

  connection_manager_->HandleMessageFromRemotePeer(PrivateGetRemotePeer(),
                                                   peer_message);
}

string PeerConnection::GetRemotePeerIdForLogging() const {
  {
    MutexLock lock(&remote_peer_mu_);
    if (remote_peer_ != NULL) {
      return remote_peer_->peer_id();
    }
  }

  return StringPrintf("(unknown peer at address %s)", remote_address_.c_str());
}

}  // namespace peer
}  // namespace floating_temple
