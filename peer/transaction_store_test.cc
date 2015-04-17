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

#include "peer/transaction_store.h"

#include <string>

#include <gflags/gflags.h>

#include "base/logging.h"
#include "fake_interpreter/fake_interpreter.h"
#include "fake_interpreter/fake_local_object.h"
#include "include/c++/thread.h"
#include "peer/canonical_peer_map.h"
#include "peer/get_peer_message_type.h"
#include "peer/interpreter_thread.h"
#include "peer/mock_peer_message_sender.h"
#include "peer/proto/peer.pb.h"
#include "third_party/gmock-1.7.0/gtest/include/gtest/gtest.h"
#include "third_party/gmock-1.7.0/include/gmock/gmock.h"

using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
using std::string;
using testing::AnyNumber;
using testing::InitGoogleMock;
using testing::_;

namespace floating_temple {
namespace peer {

class CanonicalPeer;

namespace {

MATCHER_P(IsPeerMessageType, type, "") {
  return GetPeerMessageType(arg) == type;
}

TEST(TransactionStoreTest,
     GetObjectMessagesShouldBeSentWhenNewConnectionIsReceived) {
  const string kRemotePeerId = "test-remote-peer-id";

  CanonicalPeerMap canonical_peer_map;
  MockPeerMessageSender peer_message_sender;
  FakeInterpreter interpreter;
  TransactionStore transaction_store(
      &canonical_peer_map, &peer_message_sender, &interpreter,
      canonical_peer_map.GetCanonicalPeer("test-local-peer-id"), true);

  const CanonicalPeer* const remote_peer = canonical_peer_map.GetCanonicalPeer(
      kRemotePeerId);

  EXPECT_CALL(peer_message_sender,
              BroadcastMessage(IsPeerMessageType(PeerMessage::GET_OBJECT), _))
      .Times(AnyNumber());

  // When a new connection is received, the TransactionStore instance should
  // send a GET_OBJECT message to the remote peer for each named object known to
  // the local peer.
  EXPECT_CALL(peer_message_sender,
              SendMessageToRemotePeer(
                  remote_peer, IsPeerMessageType(PeerMessage::GET_OBJECT), _))
      .Times(3);

  Thread* const thread = transaction_store.CreateInterpreterThread();

  ASSERT_TRUE(thread->BeginTransaction());
  thread->GetOrCreateNamedObject("athos", new FakeLocalObject(""));
  thread->GetOrCreateNamedObject("porthos", new FakeLocalObject(""));
  thread->GetOrCreateNamedObject("aramis", new FakeLocalObject(""));
  ASSERT_TRUE(thread->EndTransaction());

  transaction_store.NotifyNewConnection(remote_peer);
}

}  // namespace
}  // namespace peer
}  // namespace floating_temple

int main(int argc, char** argv) {
  ParseCommandLineFlags(&argc, &argv, true);
  InitGoogleLogging(argv[0]);
  InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
