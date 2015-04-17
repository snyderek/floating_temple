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

#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

#include <gflags/gflags.h>

#include "base/logging.h"
#include "base/macros.h"
#include "fake_interpreter/fake_interpreter.h"
#include "fake_interpreter/fake_local_object.h"
#include "include/c++/local_object.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "peer/canonical_peer_map.h"
#include "peer/get_peer_message_type.h"
#include "peer/interpreter_thread.h"
#include "peer/mock_peer_message_sender.h"
#include "peer/proto/peer.pb.h"
#include "third_party/gmock-1.7.0/gtest/include/gtest/gtest.h"
#include "third_party/gmock-1.7.0/include/gmock/gmock.h"

using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
using std::memcpy;
using std::size_t;
using std::string;
using std::vector;
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

class TestProgramObject : public LocalObject {
 public:
  TestProgramObject() {}

  LocalObject* Clone() const override;
  size_t Serialize(void* buffer, size_t buffer_size,
                   SerializationContext* context) const override;
  void InvokeMethod(Thread* thread,
                    PeerObject* peer_object,
                    const string& method_name,
                    const vector<Value>& parameters,
                    Value* return_value) override;
  string Dump() const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestProgramObject);
};

LocalObject* TestProgramObject::Clone() const {
  return new TestProgramObject();
}

size_t TestProgramObject::Serialize(void* buffer, size_t buffer_size,
                                    SerializationContext* context) const {
  const string kSerializedForm = "TestProgramObject:";
  const size_t length = kSerializedForm.length();

  if (length <= buffer_size) {
    memcpy(buffer, kSerializedForm.data(), length);
  }

  return length;
}

void TestProgramObject::InvokeMethod(Thread* thread,
                                     PeerObject* peer_object,
                                     const string& method_name,
                                     const vector<Value>& parameters,
                                     Value* return_value) {
  CHECK(thread != nullptr);
  CHECK_EQ(method_name, "run");
  CHECK_EQ(parameters.size(), 0);
  CHECK(return_value != nullptr);

  if (!thread->BeginTransaction()) {
    return;
  }

  thread->GetOrCreateNamedObject("athos", new FakeLocalObject(""));
  thread->GetOrCreateNamedObject("porthos", new FakeLocalObject(""));
  thread->GetOrCreateNamedObject("aramis", new FakeLocalObject(""));

  if (!thread->EndTransaction()) {
    return;
  }

  return_value->set_empty(0);
}

string TestProgramObject::Dump() const {
  return "{ \"type\": \"TestProgramObject\" }";
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

  InterpreterThread* const interpreter_thread =
      transaction_store.CreateInterpreterThread();

  Value return_value;
  interpreter_thread->RunProgram(new TestProgramObject(), "run", &return_value);
  EXPECT_EQ(Value::EMPTY, return_value.type());

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
