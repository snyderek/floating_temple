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

#include "protocol_server/protocol_connection_impl.h"

#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <string>

#include <gflags/gflags.h>

#include "base/logging.h"
#include "base/macros.h"
#include "base/notification.h"
#include "base/string_printf.h"
#include "base/thread_safe_counter.h"
#include "protocol_server/protocol_connection.h"
#include "protocol_server/protocol_connection_handler.h"
#include "protocol_server/protocol_server_interface_for_connection.h"
#include "protocol_server/testdata/test.pb.h"
#include "third_party/gmock-1.7.0/gtest/include/gtest/gtest.h"
#include "third_party/gmock-1.7.0/include/gmock/gmock.h"
#include "util/socket_util.h"

using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
using testing::AnyNumber;
using testing::DoAll;
using testing::InSequence;
using testing::InitGoogleMock;
using testing::InvokeWithoutArgs;
using testing::Return;
using testing::SetArgPointee;
using testing::Test;
using testing::_;

namespace floating_temple {
namespace {

MATCHER_P2(TestMessageMatches, n, s, "") {
  return arg.n() == n && arg.s() == s;
}

class MockProtocolServerInterfaceForConnection
    : public ProtocolServerInterfaceForConnection {
 public:
  MockProtocolServerInterfaceForConnection() {}

  MOCK_METHOD0(NotifyConnectionsChanged, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockProtocolServerInterfaceForConnection);
};

class MockProtocolConnectionHandler
    : public ProtocolConnectionHandler<TestMessage> {
 public:
  MockProtocolConnectionHandler() {}

  MOCK_METHOD1(GetNextOutputMessage, bool(TestMessage* message));
  MOCK_METHOD1(NotifyMessageReceived, void(const TestMessage& message));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockProtocolConnectionHandler);
};

bool PumpDataOnConnection(ProtocolConnectionImpl<TestMessage>* connection) {
  CHECK(connection != nullptr);

  connection->SendAndReceive();
  CHECK(!connection->close_requested());

  return !connection->IsBlocked();
}

class ProtocolConnectionImplTest : public Test {
 protected:
  void SetUp() override {
    int fds[2];
    CHECK_ERR(socketpair(AF_UNIX, SOCK_STREAM, 0, fds));

    CHECK(SetFdToNonBlocking(fds[0]));
    CHECK(SetFdToNonBlocking(fds[1]));

    protocol_connection1_ = new ProtocolConnectionImpl<TestMessage>(
        &protocol_server_, fds[0]);
    protocol_connection2_ = new ProtocolConnectionImpl<TestMessage>(
        &protocol_server_, fds[1]);

    protocol_connection1_->Init(&handler1_);
    protocol_connection2_->Init(&handler2_);
  }

  void TearDown() override {
    test_done_.Notify();

    void* thread_return_value = nullptr;
    CHECK_PTHREAD_ERR(pthread_join(data_pump_thread_, &thread_return_value));

    protocol_connection1_->CloseSocket();
    protocol_connection2_->CloseSocket();

    delete protocol_connection1_;
    delete protocol_connection2_;
  }

  void StartProtocolServer() {
    CHECK_PTHREAD_ERR(pthread_create(
        &data_pump_thread_, nullptr,
        &ProtocolConnectionImplTest::DataPumpThreadMain, this));
  }

  // Simulate the behavior of the ProtocolServer class, which repeatedly calls
  // ProtocolConnectionImpl::SendAndReceive.
  void PumpData() {
    for (;;) {
      const bool data_available1 = PumpDataOnConnection(protocol_connection1_);
      const bool data_available2 = PumpDataOnConnection(protocol_connection2_);

      if (!(data_available1 || data_available2) &&
          test_done_.WaitWithTimeout(100)) {
        return;
      }
    }
  }

  MockProtocolServerInterfaceForConnection protocol_server_;
  MockProtocolConnectionHandler handler1_;
  MockProtocolConnectionHandler handler2_;
  ProtocolConnectionImpl<TestMessage>* protocol_connection1_;
  ProtocolConnectionImpl<TestMessage>* protocol_connection2_;
  pthread_t data_pump_thread_;
  Notification test_done_;

 private:
  static void* DataPumpThreadMain(void* protocol_connection_test_raw) {
    CHECK(protocol_connection_test_raw != nullptr);
    static_cast<ProtocolConnectionImplTest*>(protocol_connection_test_raw)->
        PumpData();
    return nullptr;
  }
};

TEST_F(ProtocolConnectionImplTest, SendMessage) {
  Notification done;

  EXPECT_CALL(protocol_server_, NotifyConnectionsChanged())
      .Times(AnyNumber());

  {
    InSequence s;

    TestMessage message;
    message.set_n(123456789);
    message.set_s("abcdefg");

    EXPECT_CALL(handler1_, GetNextOutputMessage(_))
        .WillOnce(DoAll(SetArgPointee<0>(message),
                        Return(true)));

    EXPECT_CALL(handler1_, GetNextOutputMessage(_))
        .WillRepeatedly(Return(false));
  }

  EXPECT_CALL(handler2_, GetNextOutputMessage(_))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(handler2_,
              NotifyMessageReceived(TestMessageMatches(123456789, "abcdefg")))
      .WillOnce(InvokeWithoutArgs(&done, &Notification::Notify));

  StartProtocolServer();

  protocol_connection1_->NotifyMessageReadyToSend();

  ASSERT_TRUE(done.WaitWithTimeout(5000));  // milliseconds
}

TEST_F(ProtocolConnectionImplTest, SendTwoMessages) {
  Notification done;

  EXPECT_CALL(protocol_server_, NotifyConnectionsChanged())
      .Times(AnyNumber());

  {
    InSequence s;

    TestMessage message1;
    message1.set_n(1);
    message1.set_s("partridge");

    EXPECT_CALL(handler1_, GetNextOutputMessage(_))
        .WillOnce(DoAll(SetArgPointee<0>(message1),
                        Return(true)));

    TestMessage message2;
    message2.set_n(2);
    message2.set_s("turtle dove");

    EXPECT_CALL(handler1_, GetNextOutputMessage(_))
        .WillOnce(DoAll(SetArgPointee<0>(message2),
                        Return(true)));

    EXPECT_CALL(handler1_, GetNextOutputMessage(_))
        .WillRepeatedly(Return(false));
  }

  EXPECT_CALL(handler2_, GetNextOutputMessage(_))
      .WillRepeatedly(Return(false));

  {
    InSequence s;

    EXPECT_CALL(handler2_,
                NotifyMessageReceived(TestMessageMatches(1, "partridge")))
        .Times(1);
    EXPECT_CALL(handler2_,
                NotifyMessageReceived(TestMessageMatches(2, "turtle dove")))
        .WillOnce(InvokeWithoutArgs(&done, &Notification::Notify));
  }

  StartProtocolServer();

  protocol_connection1_->NotifyMessageReadyToSend();
  protocol_connection1_->NotifyMessageReadyToSend();

  ASSERT_TRUE(done.WaitWithTimeout(5000));  // milliseconds
}

void* SendTestMessage(void* protocol_connection_raw) {
  CHECK(protocol_connection_raw != nullptr);

  ProtocolConnection* const protocol_connection =
      static_cast<ProtocolConnection*>(protocol_connection_raw);

  protocol_connection->NotifyMessageReadyToSend();

  return nullptr;
}

void CreateSendTestMessageThread(ProtocolConnection* protocol_connection,
                                 pthread_t* thread) {
  CHECK(protocol_connection != nullptr);
  CHECK(thread != nullptr);

  CHECK_PTHREAD_ERR(pthread_create(thread, nullptr, &SendTestMessage,
                                   protocol_connection));
}

TEST_F(ProtocolConnectionImplTest, SendMessagesFromDifferentThreads) {
  static const int kThreadCount = 100;

  EXPECT_CALL(protocol_server_, NotifyConnectionsChanged())
      .Times(AnyNumber());

  ThreadSafeCounter counter(kThreadCount);

  {
    InSequence s;

    for (int i = 0; i < kThreadCount; ++i) {
      TestMessage message;
      message.set_n(i);
      message.set_s(StringPrintf("%d", i));

      EXPECT_CALL(handler1_, GetNextOutputMessage(_))
          .WillOnce(DoAll(SetArgPointee<0>(message),
                          Return(true)));
    }

    EXPECT_CALL(handler1_, GetNextOutputMessage(_))
        .WillRepeatedly(Return(false));
  }

  EXPECT_CALL(handler2_, GetNextOutputMessage(_))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(handler2_, NotifyMessageReceived(_))
      .Times(kThreadCount)
      .WillRepeatedly(InvokeWithoutArgs(&counter,
                                        &ThreadSafeCounter::Decrement));

  StartProtocolServer();

  pthread_t threads[kThreadCount];

  for (int i = 0; i < kThreadCount; ++i) {
    CreateSendTestMessageThread(protocol_connection1_, &threads[i]);
  }

  for (int i = 0; i < kThreadCount; ++i) {
    void* thread_return_value = nullptr;
    CHECK_PTHREAD_ERR(pthread_join(threads[i], &thread_return_value));
  }

  ASSERT_TRUE(counter.WaitForZeroWithTimeout(5000));  // milliseconds
}

}  // namespace
}  // namespace floating_temple

int main(int argc, char** argv) {
  ParseCommandLineFlags(&argc, &argv, true);
  InitGoogleLogging(argv[0]);
  InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
