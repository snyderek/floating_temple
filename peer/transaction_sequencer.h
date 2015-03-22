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

#ifndef PEER_TRANSACTION_SEQUENCER_H_
#define PEER_TRANSACTION_SEQUENCER_H_

#include <map>

#include "base/linked_ptr.h"
#include "base/macros.h"
#include "base/mutex.h"
#include "peer/peer_message_sender.h"
#include "peer/proto/peer.pb.h"
#include "peer/proto/transaction_id.pb.h"

namespace floating_temple {
namespace peer {

class CanonicalPeer;
class CanonicalPeerMap;
class PeerMessage;
class PeerMessageSender;
class TransactionIdGenerator;

class TransactionSequencer {
 public:
  TransactionSequencer(CanonicalPeerMap* canonical_peer_map,
                       PeerMessageSender* peer_message_sender,
                       TransactionIdGenerator* transaction_id_generator,
                       const CanonicalPeer* local_peer);
  ~TransactionSequencer();

  void ReserveTransaction(TransactionId* transaction_id);
  void ReleaseTransaction(const TransactionId& transaction_id);

  void SendMessageToRemotePeer(const CanonicalPeer* canonical_peer,
                               const PeerMessage& peer_message,
                               PeerMessageSender::SendMode send_mode);
  void BroadcastMessage(const PeerMessage& peer_message,
                        PeerMessageSender::SendMode send_mode);

 private:
  struct Transaction;

  struct OutgoingMessage {
    enum Type { UNICAST, BROADCAST };

    Type type;
    const CanonicalPeer* remote_peer;
    PeerMessage peer_message;
    PeerMessageSender::SendMode send_mode;
  };

  void QueueOutgoingMessage(OutgoingMessage::Type type,
                            const CanonicalPeer* remote_peer,
                            const PeerMessage& peer_message,
                            PeerMessageSender::SendMode send_mode);
  void FlushMessages_Locked();
  void SendOutgoingMessage(const OutgoingMessage& outgoing_message);

  const TransactionId* ExtractTransactionIdFromPeerMessage(
      const PeerMessage& peer_message) const;

  CanonicalPeerMap* const canonical_peer_map_;
  PeerMessageSender* const peer_message_sender_;
  // TODO(dss): Fold TransactionIdGenerator into this class.
  TransactionIdGenerator* const transaction_id_generator_;
  const CanonicalPeer* const local_peer_;

  std::map<TransactionId, linked_ptr<Transaction>> transactions_;
  mutable Mutex mu_;

  DISALLOW_COPY_AND_ASSIGN(TransactionSequencer);
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_TRANSACTION_SEQUENCER_H_
