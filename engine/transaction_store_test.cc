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

#include "engine/transaction_store.h"

#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

#include <gflags/gflags.h>

#include "base/logging.h"
#include "base/macros.h"
#include "engine/canonical_peer_map.h"
#include "engine/get_peer_message_type.h"
#include "engine/mock_peer_message_sender.h"
#include "engine/proto/peer.pb.h"
#include "fake_interpreter/fake_interpreter.h"
#include "fake_interpreter/fake_local_object.h"
#include "include/c++/local_object.h"
#include "include/c++/method_context.h"
#include "include/c++/value.h"
#include "third_party/gmock-1.7.0/gtest/include/gtest/gtest.h"
#include "third_party/gmock-1.7.0/include/gmock/gmock.h"
#include "util/dump_context.h"

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
namespace engine {

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
  void InvokeMethod(MethodContext* method_context,
                    ObjectReference* self_object_reference,
                    const string& method_name,
                    const vector<Value>& parameters,
                    Value* return_value) override;
  void Dump(DumpContext* dc) const override;

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

void TestProgramObject::InvokeMethod(MethodContext* method_context,
                                     ObjectReference* self_object_reference,
                                     const string& method_name,
                                     const vector<Value>& parameters,
                                     Value* return_value) {
  CHECK(method_context != nullptr);
  CHECK_EQ(method_name, "run");
  CHECK_EQ(parameters.size(), 0);
  CHECK(return_value != nullptr);

  if (!method_context->BeginTransaction()) {
    return;
  }

  method_context->CreateObject(new FakeLocalObject(""), "athos");
  method_context->CreateObject(new FakeLocalObject(""), "porthos");
  method_context->CreateObject(new FakeLocalObject(""), "aramis");

  if (!method_context->EndTransaction()) {
    return;
  }

  return_value->set_empty(0);
}

void TestProgramObject::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();
  dc->AddString("type");
  dc->AddString("TestProgramObject");
  dc->End();
}

TEST(TransactionStoreTest,
     GetObjectMessagesShouldBeSentWhenNewConnectionIsReceived) {
  const string kRemotePeerId = "test-remote-peer-id";

  CanonicalPeerMap canonical_peer_map;
  MockPeerMessageSender peer_message_sender;
  FakeInterpreter interpreter;
  TransactionStore transaction_store(
      &canonical_peer_map, &peer_message_sender, &interpreter,
      canonical_peer_map.GetCanonicalPeer("test-local-peer-id"));

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

  Value return_value;
  transaction_store.RunProgram(new TestProgramObject(), "run", &return_value,
                               false);
  EXPECT_EQ(Value::EMPTY, return_value.type());

  transaction_store.NotifyNewConnection(remote_peer);
}

}  // namespace
}  // namespace engine
}  // namespace floating_temple

int main(int argc, char** argv) {
  ParseCommandLineFlags(&argc, &argv, true);
  InitGoogleLogging(argv[0]);
  InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
