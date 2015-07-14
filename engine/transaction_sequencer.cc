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

#include "engine/transaction_sequencer.h"

#include <map>
#include <string>
#include <vector>

#include "base/linked_ptr.h"
#include "base/logging.h"
#include "base/mutex_lock.h"
#include "engine/canonical_peer.h"
#include "engine/canonical_peer_map.h"
#include "engine/get_peer_message_type.h"
#include "engine/peer_message_sender.h"
#include "engine/proto/peer.pb.h"
#include "engine/proto/transaction_id.pb.h"
#include "engine/transaction_id_generator.h"
#include "engine/transaction_id_util.h"

using std::map;
using std::vector;

namespace floating_temple {
namespace peer {

struct TransactionSequencer::Transaction {
  vector<linked_ptr<OutgoingMessage>> outgoing_messages;
  bool done;
};

TransactionSequencer::TransactionSequencer(
    CanonicalPeerMap* canonical_peer_map,
    PeerMessageSender* peer_message_sender,
    TransactionIdGenerator* transaction_id_generator,
    const CanonicalPeer* local_peer)
    : canonical_peer_map_(CHECK_NOTNULL(canonical_peer_map)),
      peer_message_sender_(CHECK_NOTNULL(peer_message_sender)),
      transaction_id_generator_(CHECK_NOTNULL(transaction_id_generator)),
      local_peer_(CHECK_NOTNULL(local_peer)) {
}

TransactionSequencer::~TransactionSequencer() {
}

void TransactionSequencer::ReserveTransaction(TransactionId* transaction_id) {
  CHECK(transaction_id != nullptr);

  MutexLock lock(&mu_);

  transaction_id_generator_->Generate(transaction_id);

  if (!transactions_.empty()) {
    CHECK_LT(CompareTransactionIds(transactions_.rbegin()->first,
                                   *transaction_id), 0);
  }

  Transaction* const transaction = new Transaction();
  transaction->done = false;
  // TODO(dss): Provide a hint to map<...>::emplace that the new map entry is
  // being inserted at the end.
  CHECK(transactions_.emplace(*transaction_id,
                              make_linked_ptr(transaction)).second);
}

void TransactionSequencer::ReleaseTransaction(
    const TransactionId& transaction_id) {
  MutexLock lock(&mu_);

  const map<TransactionId, linked_ptr<Transaction>>::iterator it =
      transactions_.find(transaction_id);
  CHECK(it != transactions_.end());

  Transaction* const transaction = it->second.get();
  CHECK(!transaction->done);
  transaction->done = true;

  FlushMessages_Locked();
}

void TransactionSequencer::SendMessageToRemotePeer(
    const CanonicalPeer* canonical_peer, const PeerMessage& peer_message,
    PeerMessageSender::SendMode send_mode) {
  QueueOutgoingMessage(OutgoingMessage::UNICAST, canonical_peer, peer_message,
                       send_mode);
}

void TransactionSequencer::BroadcastMessage(
    const PeerMessage& peer_message, PeerMessageSender::SendMode send_mode) {
  QueueOutgoingMessage(OutgoingMessage::BROADCAST, nullptr, peer_message,
                       send_mode);
}

void TransactionSequencer::QueueOutgoingMessage(
    OutgoingMessage::Type type,
    const CanonicalPeer* remote_peer,
    const PeerMessage& peer_message,
    PeerMessageSender::SendMode send_mode) {
  OutgoingMessage* const outgoing_message = new OutgoingMessage();
  outgoing_message->type = type;
  outgoing_message->remote_peer = remote_peer;
  // TODO(dss): As an optimization, don't make a copy of the peer message if it
  // can be sent immediately.
  outgoing_message->peer_message.CopyFrom(peer_message);
  outgoing_message->send_mode = send_mode;

  const TransactionId* const transaction_id =
      ExtractTransactionIdFromPeerMessage(peer_message);

  if (transaction_id == nullptr) {
    SendOutgoingMessage(*outgoing_message);
    delete outgoing_message;
  } else {
    MutexLock lock(&mu_);

    const map<TransactionId, linked_ptr<Transaction>>::iterator it =
        transactions_.find(*transaction_id);
    CHECK(it != transactions_.end());
    it->second->outgoing_messages.emplace_back(outgoing_message);

    FlushMessages_Locked();
  }
}

void TransactionSequencer::FlushMessages_Locked() {
  for (;;) {
    const map<TransactionId, linked_ptr<Transaction>>::iterator transaction_it =
        transactions_.begin();
    if (transaction_it == transactions_.end()) {
      return;
    }

    const Transaction& transaction = *transaction_it->second;

    for (const auto& message : transaction.outgoing_messages) {
      SendOutgoingMessage(*message);
    }

    if (!transaction.done) {
      return;
    }

    transactions_.erase(transaction_it);
  }
}

void TransactionSequencer::SendOutgoingMessage(
    const OutgoingMessage& outgoing_message) {
  const OutgoingMessage::Type type = outgoing_message.type;
  const CanonicalPeer* remote_peer = outgoing_message.remote_peer;
  const PeerMessage& peer_message = outgoing_message.peer_message;
  const PeerMessageSender::SendMode send_mode = outgoing_message.send_mode;

  switch (type) {
    case OutgoingMessage::UNICAST:
      CHECK(remote_peer != nullptr);
      peer_message_sender_->SendMessageToRemotePeer(remote_peer, peer_message,
                                                    send_mode);
      break;

    case OutgoingMessage::BROADCAST:
      CHECK(remote_peer == nullptr);
      peer_message_sender_->BroadcastMessage(peer_message, send_mode);
      break;

    default:
      LOG(FATAL) << "Invalid OutgoingMessage type: " << static_cast<int>(type);
  }
}

const TransactionId* TransactionSequencer::ExtractTransactionIdFromPeerMessage(
    const PeerMessage& peer_message) const {
  switch (GetPeerMessageType(peer_message)) {
    case PeerMessage::APPLY_TRANSACTION: {
      const ApplyTransactionMessage& apply_transaction_message =
          peer_message.apply_transaction_message();
      return &apply_transaction_message.transaction_id();
    }

    case PeerMessage::REJECT_TRANSACTION: {
      const RejectTransactionMessage& reject_transaction_message =
          peer_message.reject_transaction_message();
      return &reject_transaction_message.new_transaction_id();
    }

    case PeerMessage::INVALIDATE_TRANSACTIONS: {
      const InvalidateTransactionsMessage& invalidate_transactions_message =
          peer_message.invalidate_transactions_message();
      return &invalidate_transactions_message.end_transaction_id();
    }

    default:
      return nullptr;
  }
}

}  // namespace peer
}  // namespace floating_temple
