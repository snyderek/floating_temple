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

#include <pthread.h>

#include <string>

#include <gflags/gflags.h>

#include "base/logging.h"
#include "base/notification.h"
#include "base/string_printf.h"
#include "engine/canonical_peer_map.h"
#include "engine/get_peer_message_type.h"
#include "engine/mock_connection_handler.h"
#include "engine/peer_id.h"
#include "engine/peer_message_sender.h"
#include "engine/proto/peer.pb.h"
#include "third_party/gmock-1.7.0/gtest/include/gtest/gtest.h"
#include "third_party/gmock-1.7.0/include/gmock/gmock.h"
#include "util/tcp.h"

using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
using std::string;
using testing::AtLeast;
using testing::AtMost;
using testing::InSequence;
using testing::InitGoogleMock;
using testing::InvokeWithoutArgs;
using testing::_;

namespace floating_temple {
namespace peer {

class CanonicalPeer;

namespace {

MATCHER_P(HasTestMessageText, text, "") {
  CHECK_EQ(GetPeerMessageType(arg), PeerMessage::TEST);
  return arg.test_message().text() == text;
}

TEST(ConnectionManagerTest, SendMessageToRemotePeer) {
  Notification done;

  MockConnectionHandler connection_handler1;
  MockConnectionHandler connection_handler2;

  {
    InSequence s;

    EXPECT_CALL(connection_handler1, NotifyNewConnection(_))
        .Times(AtMost(1));
  }

  {
    InSequence s;

    EXPECT_CALL(connection_handler2, NotifyNewConnection(_));
    EXPECT_CALL(
        connection_handler2,
        HandleMessageFromRemotePeer(_, HasTestMessageText("test message 1")));
    EXPECT_CALL(
        connection_handler2,
        HandleMessageFromRemotePeer(_, HasTestMessageText("test message 2")));
    EXPECT_CALL(
        connection_handler2,
        HandleMessageFromRemotePeer(_, HasTestMessageText("test message 3")))
        .WillOnce(InvokeWithoutArgs(&done, &Notification::Notify));
  }

  CanonicalPeerMap canonical_peer_map;

  const string local_address = GetLocalAddress();
  const CanonicalPeer* const canonical_peer1 =
      canonical_peer_map.GetCanonicalPeer(
      MakePeerId(local_address, GetUnusedPortForTesting()));
  const CanonicalPeer* const canonical_peer2 =
      canonical_peer_map.GetCanonicalPeer(
      MakePeerId(local_address, GetUnusedPortForTesting()));

  ConnectionManager connection_manager1, connection_manager2;

  connection_manager1.Start(&canonical_peer_map, "test-interpreter-type",
                            canonical_peer1, &connection_handler1, 1);
  connection_manager2.Start(&canonical_peer_map, "test-interpreter-type",
                            canonical_peer2, &connection_handler2, 1);

  for (int i = 1; i <= 3; ++i) {
    PeerMessage peer_message;
    peer_message.mutable_test_message()->set_text(
        StringPrintf("test message %d", i));

    connection_manager1.SendMessageToRemotePeer(
        canonical_peer2, peer_message, PeerMessageSender::NON_BLOCKING_MODE);
  }

  ASSERT_TRUE(done.WaitWithTimeout(5000));  // milliseconds

  connection_manager1.Stop();
  connection_manager2.Stop();
}

struct SendTestMessageInfo {
  PeerMessageSender* peer_message_sender;
  const CanonicalPeer* remote_peer;
  string message;
};

void* SendTestMessage(void* info_raw) {
  CHECK(info_raw != nullptr);

  const SendTestMessageInfo* const info = static_cast<SendTestMessageInfo*>(
      info_raw);

  PeerMessage peer_message;
  peer_message.mutable_test_message()->set_text(info->message);

  info->peer_message_sender->SendMessageToRemotePeer(
      info->remote_peer, peer_message, PeerMessageSender::NON_BLOCKING_MODE);

  delete info;
  return nullptr;
}

void CreateSendTestMessageThread(PeerMessageSender* peer_message_sender,
                                 const CanonicalPeer* remote_peer,
                                 const string& message,
                                 pthread_t* thread) {
  CHECK(peer_message_sender != nullptr);
  CHECK(thread != nullptr);

  SendTestMessageInfo* info = new SendTestMessageInfo();
  info->peer_message_sender = peer_message_sender;
  info->remote_peer = remote_peer;
  info->message = message;

  CHECK_PTHREAD_ERR(pthread_create(thread, nullptr, &SendTestMessage, info));
}

TEST(ConnectionManagerTest, SimultaneousConnections) {
  Notification done1;
  Notification done2;

  MockConnectionHandler connection_handler1;
  MockConnectionHandler connection_handler2;

  EXPECT_CALL(connection_handler1, NotifyNewConnection(_))
      .Times(AtLeast(1));
  EXPECT_CALL(connection_handler2, NotifyNewConnection(_))
      .Times(AtLeast(1));

  EXPECT_CALL(
      connection_handler1,
      HandleMessageFromRemotePeer(_, HasTestMessageText("Guilder")))
      .WillOnce(InvokeWithoutArgs(&done1, &Notification::Notify));
  EXPECT_CALL(
      connection_handler2,
      HandleMessageFromRemotePeer(_, HasTestMessageText("Florin")))
      .WillOnce(InvokeWithoutArgs(&done2, &Notification::Notify));

  CanonicalPeerMap canonical_peer_map;

  const string local_address = GetLocalAddress();
  const CanonicalPeer* const canonical_peer1 =
      canonical_peer_map.GetCanonicalPeer(
      MakePeerId(local_address, GetUnusedPortForTesting()));
  const CanonicalPeer* const canonical_peer2 =
      canonical_peer_map.GetCanonicalPeer(
      MakePeerId(local_address, GetUnusedPortForTesting()));

  ConnectionManager connection_manager1, connection_manager2;

  connection_manager1.Start(&canonical_peer_map, "test-interpreter-type",
                            canonical_peer1, &connection_handler1, 1);
  connection_manager2.Start(&canonical_peer_map, "test-interpreter-type",
                            canonical_peer2, &connection_handler2, 1);

  pthread_t thread1, thread2;
  CreateSendTestMessageThread(&connection_manager1, canonical_peer2, "Florin",
                              &thread1);
  CreateSendTestMessageThread(&connection_manager2, canonical_peer1, "Guilder",
                              &thread2);

  void* thread_return_value = nullptr;
  CHECK_PTHREAD_ERR(pthread_join(thread1, &thread_return_value));
  CHECK_PTHREAD_ERR(pthread_join(thread2, &thread_return_value));

  ASSERT_TRUE(done1.WaitWithTimeout(5000));  // milliseconds
  ASSERT_TRUE(done2.WaitWithTimeout(5000));

  connection_manager1.Stop();
  connection_manager2.Stop();
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
