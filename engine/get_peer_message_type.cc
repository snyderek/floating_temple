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

#include "engine/get_peer_message_type.h"

#include "base/logging.h"
#include "engine/proto/peer.pb.h"

namespace floating_temple {
namespace engine {

#define CHECK_FIELD(has_method, enum_const) \
  if (peer_message.has_method()) { \
    CHECK_EQ(type, PeerMessage::UNKNOWN); \
    type = PeerMessage::enum_const; \
  }

PeerMessage::Type GetPeerMessageType(const PeerMessage& peer_message) {
  PeerMessage::Type type = PeerMessage::UNKNOWN;

  CHECK_FIELD(has_hello_message, HELLO);
  CHECK_FIELD(has_goodbye_message, GOODBYE);
  CHECK_FIELD(has_apply_transaction_message, APPLY_TRANSACTION);
  CHECK_FIELD(has_get_object_message, GET_OBJECT);
  CHECK_FIELD(has_store_object_message, STORE_OBJECT);
  CHECK_FIELD(has_reject_transaction_message, REJECT_TRANSACTION);
  CHECK_FIELD(has_invalidate_transactions_message, INVALIDATE_TRANSACTIONS);
  CHECK_FIELD(has_test_message, TEST);

  CHECK_NE(type, PeerMessage::UNKNOWN);

  return type;
}

#undef CHECK_FIELD

}  // namespace engine
}  // namespace floating_temple
