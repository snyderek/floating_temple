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

#ifndef ENGINE_PEER_MESSAGE_SENDER_H_
#define ENGINE_PEER_MESSAGE_SENDER_H_

namespace floating_temple {
namespace peer {

class CanonicalPeer;
class PeerMessage;

class PeerMessageSender {
 public:
  enum SendMode {
    NON_BLOCKING_MODE,

    // Blocking mode doesn't actually wait for the message to be sent; it just
    // waits for the message to be queued. This is useful for throttling
    // messages that originate from the local interpreter, to prevent them from
    // exhausting the available memory.
    BLOCKING_MODE
  };

  virtual ~PeerMessageSender() {}

  virtual void SendMessageToRemotePeer(const CanonicalPeer* canonical_peer,
                                       const PeerMessage& peer_message,
                                       SendMode send_mode) = 0;
  virtual void BroadcastMessage(const PeerMessage& peer_message,
                                SendMode send_mode) = 0;
};

}  // namespace peer
}  // namespace floating_temple

#endif  // ENGINE_PEER_MESSAGE_SENDER_H_
