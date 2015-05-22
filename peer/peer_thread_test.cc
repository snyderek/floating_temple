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

#include "peer/peer_thread.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <gflags/gflags.h>

#include "base/integral_types.h"
#include "base/logging.h"
#include "fake_interpreter/fake_local_object.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "peer/committed_event.h"
#include "peer/committed_value.h"
#include "peer/const_live_object_ptr.h"
#include "peer/live_object.h"
#include "peer/live_object_ptr.h"
#include "peer/mock_local_object.h"
#include "peer/mock_transaction_store.h"
#include "peer/peer_object_impl.h"
#include "peer/proto/uuid.pb.h"
#include "peer/shared_object.h"
#include "third_party/gmock-1.7.0/gtest/include/gtest/gtest.h"
#include "third_party/gmock-1.7.0/include/gmock/gmock.h"

using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
using std::unordered_map;
using std::unordered_set;
using std::vector;
using testing::AnyNumber;
using testing::InitGoogleMock;
using testing::Invoke;
using testing::SetArgPointee;
using testing::WithArgs;
using testing::_;

namespace floating_temple {
namespace peer {
namespace {

Uuid MakeUuid(int n) {
  CHECK_GT(n, 0);

  Uuid uuid;
  uuid.set_high_word(static_cast<uint64>(n));
  uuid.set_low_word(0);

  return uuid;
}

void TestMethod1(Thread* thread, const vector<Value>& parameters,
                 Value* return_value) {
  CHECK(thread != nullptr);
  CHECK_EQ(parameters.size(), 1u);
  CHECK(return_value != nullptr);

  Value sub_return_value;
  if (!thread->CallMethod(parameters[0].peer_object(), "test_method2",
                          vector<Value>(), &sub_return_value)) {
    return;
  }

  CHECK_NE(sub_return_value.type(), Value::UNINITIALIZED);

  return_value->set_empty(0);
}

TEST(PeerThreadTest, SubMethodCallWithoutReturn) {
  MockTransactionStoreCore transaction_store_core;
  MockTransactionStore transaction_store(&transaction_store_core);
  SharedObject shared_object1(&transaction_store, MakeUuid(1));
  SharedObject shared_object2(&transaction_store, MakeUuid(2));
  const MockLocalObjectCore local_object_core1;
  LiveObjectPtr live_object1(
      new LiveObject(new MockLocalObject(&local_object_core1)));

  EXPECT_CALL(transaction_store_core, GetCurrentSequencePoint())
      .Times(0);
  EXPECT_CALL(transaction_store_core, GetLiveObjectAtSequencePoint(_, _, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, CreateUnboundPeerObject(_))
      .Times(AnyNumber());
  EXPECT_CALL(transaction_store_core, CreateBoundPeerObject(_, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, CreateTransaction(_, _, _, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, ObjectsAreEquivalent(_, _))
      .Times(0);

  EXPECT_CALL(local_object_core1, Serialize(_))
      .Times(0);
  EXPECT_CALL(local_object_core1, InvokeMethod(_, _, "test_method1", _, _))
      .WillRepeatedly(WithArgs<0, 3, 4>(Invoke(&TestMethod1)));

  const unordered_set<SharedObject*> new_shared_objects;

  vector<CommittedValue> method_parameters(1);
  method_parameters[0].set_local_type(0);
  method_parameters[0].set_shared_object(&shared_object2);
  const MethodCallCommittedEvent event1(nullptr, "test_method1",
                                        method_parameters);

  const SubMethodCallCommittedEvent event2(new_shared_objects, &shared_object2,
                                           "test_method2",
                                           vector<CommittedValue>());

  unordered_map<SharedObject*, PeerObjectImpl*> new_peer_objects;

  PeerThread peer_thread;
  peer_thread.Start(&transaction_store, &shared_object1, live_object1,
                    &new_peer_objects);

  peer_thread.QueueEvent(&event1);
  peer_thread.QueueEvent(&event2);

  peer_thread.Stop();

  EXPECT_FALSE(peer_thread.conflict_detected());
}

TEST(PeerThreadTest, FlushEvents) {
  MockTransactionStoreCore transaction_store_core;
  MockTransactionStore transaction_store(&transaction_store_core);
  SharedObject shared_object(&transaction_store, MakeUuid(111));
  const MockLocalObjectCore local_object_core;
  LiveObjectPtr live_object(
      new LiveObject(new MockLocalObject(&local_object_core)));

  EXPECT_CALL(transaction_store_core, GetCurrentSequencePoint())
      .Times(0);
  EXPECT_CALL(transaction_store_core, GetLiveObjectAtSequencePoint(_, _, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, CreateUnboundPeerObject(_))
      .Times(AnyNumber());
  EXPECT_CALL(transaction_store_core, CreateBoundPeerObject(_, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, CreateTransaction(_, _, _, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, ObjectsAreEquivalent(_, _))
      .Times(0);

  Value canned_return_value;
  canned_return_value.set_empty(0);

  EXPECT_CALL(local_object_core, Serialize(_))
      .Times(0);
  EXPECT_CALL(local_object_core, InvokeMethod(_, _, "test_method2", _, _))
      .WillRepeatedly(SetArgPointee<4>(canned_return_value));

  const unordered_set<SharedObject*> new_shared_objects;

  const MethodCallCommittedEvent event1(nullptr, "test_method2",
                                        vector<CommittedValue>());

  CommittedValue expected_return_value;
  expected_return_value.set_local_type(0);
  expected_return_value.set_empty();

  const MethodReturnCommittedEvent event2(new_shared_objects, nullptr,
                                          expected_return_value);

  unordered_map<SharedObject*, PeerObjectImpl*> new_peer_objects;

  PeerThread peer_thread;
  peer_thread.Start(&transaction_store, &shared_object, live_object,
                    &new_peer_objects);

  peer_thread.QueueEvent(&event1);
  peer_thread.QueueEvent(&event2);

  peer_thread.FlushEvents();

  peer_thread.Stop();

  EXPECT_FALSE(peer_thread.conflict_detected());
}

TEST(PeerThreadTest, MultipleTransactions) {
  MockTransactionStoreCore transaction_store_core;
  MockTransactionStore transaction_store(&transaction_store_core);
  SharedObject shared_object(&transaction_store, MakeUuid(222));
  FakeLocalObject* const local_object = new FakeLocalObject("snap.");
  LiveObjectPtr live_object(new LiveObject(local_object));

  EXPECT_CALL(transaction_store_core, GetCurrentSequencePoint())
      .Times(0);
  EXPECT_CALL(transaction_store_core, GetLiveObjectAtSequencePoint(_, _, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, CreateUnboundPeerObject(_))
      .Times(AnyNumber());
  EXPECT_CALL(transaction_store_core, CreateBoundPeerObject(_, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, CreateTransaction(_, _, _, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, ObjectsAreEquivalent(_, _))
      .Times(0);

  const unordered_set<SharedObject*> new_shared_objects;

  CommittedValue empty_return_value;
  empty_return_value.set_local_type(FakeLocalObject::kVoidLocalType);
  empty_return_value.set_empty();

  vector<CommittedValue> event1_parameters(1);
  event1_parameters[0].set_local_type(FakeLocalObject::kStringLocalType);
  event1_parameters[0].set_string_value("crackle.");

  const MethodCallCommittedEvent event1(nullptr, "append", event1_parameters);
  const MethodReturnCommittedEvent event2(new_shared_objects, nullptr,
                                          empty_return_value);

  vector<CommittedValue> event3_parameters(1);
  event3_parameters[0].set_local_type(FakeLocalObject::kStringLocalType);
  event3_parameters[0].set_string_value("pop.");

  const MethodCallCommittedEvent event3(nullptr, "append", event3_parameters);
  const MethodReturnCommittedEvent event4(new_shared_objects, nullptr,
                                          empty_return_value);

  unordered_map<SharedObject*, PeerObjectImpl*> new_peer_objects;

  PeerThread peer_thread;
  peer_thread.Start(&transaction_store, &shared_object, live_object,
                    &new_peer_objects);

  peer_thread.QueueEvent(&event1);
  peer_thread.QueueEvent(&event2);

  peer_thread.FlushEvents();

  peer_thread.QueueEvent(&event3);
  peer_thread.QueueEvent(&event4);

  peer_thread.Stop();

  EXPECT_FALSE(peer_thread.conflict_detected());
  EXPECT_EQ(0u, new_peer_objects.size());
  EXPECT_EQ("snap.crackle.pop.", local_object->s());
}

TEST(PeerThreadTest, TransactionAfterConflictDetected) {
  MockTransactionStoreCore transaction_store_core;
  MockTransactionStore transaction_store(&transaction_store_core);
  SharedObject shared_object(&transaction_store, MakeUuid(333));
  LiveObjectPtr live_object(new LiveObject(new FakeLocalObject("peter.")));

  EXPECT_CALL(transaction_store_core, GetCurrentSequencePoint())
      .Times(0);
  EXPECT_CALL(transaction_store_core, GetLiveObjectAtSequencePoint(_, _, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, CreateUnboundPeerObject(_))
      .Times(AnyNumber());
  EXPECT_CALL(transaction_store_core, CreateBoundPeerObject(_, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, CreateTransaction(_, _, _, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, ObjectsAreEquivalent(_, _))
      .Times(0);

  const unordered_set<SharedObject*> new_shared_objects;

  CommittedValue empty_return_value;
  empty_return_value.set_local_type(FakeLocalObject::kVoidLocalType);
  empty_return_value.set_empty();

  vector<CommittedValue> event1_parameters(1);
  event1_parameters[0].set_local_type(FakeLocalObject::kStringLocalType);
  event1_parameters[0].set_string_value("paul.");

  const MethodCallCommittedEvent event1(nullptr, "append", event1_parameters);
  const MethodReturnCommittedEvent event2(new_shared_objects, nullptr,
                                          empty_return_value);

  CommittedValue bogus_return_value;
  bogus_return_value.set_local_type(FakeLocalObject::kStringLocalType);
  bogus_return_value.set_string_value("barney.");

  const MethodCallCommittedEvent event3(nullptr, "get",
                                        vector<CommittedValue>());
  // This event should cause a conflict.
  const MethodReturnCommittedEvent event4(new_shared_objects, nullptr,
                                          bogus_return_value);

  vector<CommittedValue> event5_parameters(1);
  event5_parameters[0].set_local_type(FakeLocalObject::kStringLocalType);
  event5_parameters[0].set_string_value("mary.");

  const MethodCallCommittedEvent event5(nullptr, "append", event5_parameters);
  const MethodReturnCommittedEvent event6(new_shared_objects, nullptr,
                                          empty_return_value);

  unordered_map<SharedObject*, PeerObjectImpl*> new_peer_objects;

  PeerThread peer_thread;
  peer_thread.Start(&transaction_store, &shared_object, live_object,
                    &new_peer_objects);

  peer_thread.QueueEvent(&event1);
  peer_thread.QueueEvent(&event2);
  peer_thread.QueueEvent(&event3);
  peer_thread.QueueEvent(&event4);

  peer_thread.FlushEvents();

  // Keep queuing events even though a conflict has occurred. The PeerThread
  // instance should quietly ignore these events.
  peer_thread.QueueEvent(&event5);
  peer_thread.QueueEvent(&event6);

  peer_thread.Stop();

  EXPECT_TRUE(peer_thread.conflict_detected());
  EXPECT_EQ(0u, new_peer_objects.size());
}

TEST(PeerThreadTest, MethodCallWithoutReturn) {
  MockTransactionStoreCore transaction_store_core;
  MockTransactionStore transaction_store(&transaction_store_core);
  SharedObject shared_object(&transaction_store, MakeUuid(1));
  const MockLocalObjectCore local_object_core;
  LiveObjectPtr live_object(
      new LiveObject(new MockLocalObject(&local_object_core)));

  EXPECT_CALL(transaction_store_core, GetCurrentSequencePoint())
      .Times(0);
  EXPECT_CALL(transaction_store_core, GetLiveObjectAtSequencePoint(_, _, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, CreateUnboundPeerObject(_))
      .Times(AnyNumber());
  EXPECT_CALL(transaction_store_core, CreateBoundPeerObject(_, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, CreateTransaction(_, _, _, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, ObjectsAreEquivalent(_, _))
      .Times(0);

  Value canned_return_value;
  canned_return_value.set_empty(0);

  EXPECT_CALL(local_object_core, Serialize(_))
      .Times(0);
  EXPECT_CALL(local_object_core, InvokeMethod(_, _, "test_method1", _, _))
      .WillRepeatedly(SetArgPointee<4>(canned_return_value));
  // The sequence of events to be replayed ends with a method call to
  // test_method2, and so the method itself should not be executed.
  EXPECT_CALL(local_object_core, InvokeMethod(_, _, "test_method2", _, _))
      .Times(0);

  CommittedValue expected_return_value;
  expected_return_value.set_local_type(0);
  expected_return_value.set_empty();

  const unordered_set<SharedObject*> new_shared_objects;

  const MethodCallCommittedEvent event1(nullptr, "test_method1",
                                        vector<CommittedValue>());
  const MethodReturnCommittedEvent event2(new_shared_objects, nullptr,
                                          expected_return_value);
  const MethodCallCommittedEvent event3(nullptr, "test_method2",
                                        vector<CommittedValue>());

  unordered_map<SharedObject*, PeerObjectImpl*> new_peer_objects;

  PeerThread peer_thread;
  peer_thread.Start(&transaction_store, &shared_object, live_object,
                    &new_peer_objects);
  peer_thread.QueueEvent(&event1);
  peer_thread.QueueEvent(&event2);
  peer_thread.QueueEvent(&event3);
  peer_thread.Stop();

  EXPECT_FALSE(peer_thread.conflict_detected());
}

TEST(PeerThreadTest, SelfMethodCallWithoutReturn) {
  MockTransactionStoreCore transaction_store_core;
  MockTransactionStore transaction_store(&transaction_store_core);
  SharedObject shared_object(&transaction_store, MakeUuid(1));
  const MockLocalObjectCore local_object_core;
  LiveObjectPtr live_object(
      new LiveObject(new MockLocalObject(&local_object_core)));

  EXPECT_CALL(transaction_store_core, GetCurrentSequencePoint())
      .Times(0);
  EXPECT_CALL(transaction_store_core, GetLiveObjectAtSequencePoint(_, _, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, CreateUnboundPeerObject(_))
      .Times(AnyNumber());
  EXPECT_CALL(transaction_store_core, CreateBoundPeerObject(_, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, CreateTransaction(_, _, _, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, ObjectsAreEquivalent(_, _))
      .Times(0);

  EXPECT_CALL(local_object_core, Serialize(_))
      .Times(0);
  EXPECT_CALL(local_object_core, InvokeMethod(_, _, "test_method1", _, _))
      .WillRepeatedly(WithArgs<0, 3, 4>(Invoke(&TestMethod1)));
  // The sequence of events to be replayed ends with a method call to
  // test_method2, and so the method itself should not be executed.
  EXPECT_CALL(local_object_core, InvokeMethod(_, _, "test_method2", _, _))
      .Times(0);

  const unordered_set<SharedObject*> new_shared_objects;

  vector<CommittedValue> method_parameters(1);
  method_parameters[0].set_local_type(0);
  method_parameters[0].set_shared_object(&shared_object);
  const MethodCallCommittedEvent event1(nullptr, "test_method1",
                                        method_parameters);

  const SelfMethodCallCommittedEvent event2(new_shared_objects, "test_method2",
                                            vector<CommittedValue>());

  unordered_map<SharedObject*, PeerObjectImpl*> new_peer_objects;

  PeerThread peer_thread;
  peer_thread.Start(&transaction_store, &shared_object, live_object,
                    &new_peer_objects);

  peer_thread.QueueEvent(&event1);
  peer_thread.QueueEvent(&event2);

  peer_thread.Stop();

  EXPECT_FALSE(peer_thread.conflict_detected());
}

void TestMethod3(Thread* thread, const vector<Value>& parameters,
                 Value* return_value) {
  CHECK(thread != nullptr);
  CHECK_EQ(parameters.size(), 1u);
  CHECK(return_value != nullptr);

  if (!thread->BeginTransaction()) {
    return;
  }

  Value sub_return_value;
  if (!thread->CallMethod(parameters[0].peer_object(), "test_method4",
                          vector<Value>(), &sub_return_value)) {
    return;
  }

  if (!thread->EndTransaction()) {
    return;
  }

  CHECK_NE(sub_return_value.type(), Value::UNINITIALIZED);

  return_value->set_empty(0);
}

TEST(PeerThreadTest, TransactionInsideMethodCall) {
  MockTransactionStoreCore transaction_store_core;
  MockTransactionStore transaction_store(&transaction_store_core);
  SharedObject shared_object1(&transaction_store, MakeUuid(1));
  SharedObject shared_object2(&transaction_store, MakeUuid(2));
  const MockLocalObjectCore local_object_core1;
  LiveObjectPtr live_object1(
      new LiveObject(new MockLocalObject(&local_object_core1)));

  EXPECT_CALL(transaction_store_core, GetCurrentSequencePoint())
      .Times(0);
  EXPECT_CALL(transaction_store_core, GetLiveObjectAtSequencePoint(_, _, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, CreateUnboundPeerObject(_))
      .Times(AnyNumber());
  EXPECT_CALL(transaction_store_core, CreateBoundPeerObject(_, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, CreateTransaction(_, _, _, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, ObjectsAreEquivalent(_, _))
      .Times(0);

  EXPECT_CALL(local_object_core1, Serialize(_))
      .Times(0);
  EXPECT_CALL(local_object_core1, InvokeMethod(_, _, "test_method3", _, _))
      .WillOnce(WithArgs<0, 3, 4>(Invoke(&TestMethod3)));

  const unordered_set<SharedObject*> new_shared_objects;

  vector<CommittedValue> method_parameters(1);
  method_parameters[0].set_local_type(0);
  method_parameters[0].set_shared_object(&shared_object2);

  CommittedValue empty_return_value;
  empty_return_value.set_local_type(0);
  empty_return_value.set_empty();

  const MethodCallCommittedEvent event1(nullptr, "test_method3",
                                        method_parameters);
  const BeginTransactionCommittedEvent event2;
  const SubMethodCallCommittedEvent event3(new_shared_objects, &shared_object2,
                                           "test_method4",
                                           vector<CommittedValue>());
  const SubMethodReturnCommittedEvent event4(&shared_object2,
                                             empty_return_value);
  const EndTransactionCommittedEvent event5;
  const MethodReturnCommittedEvent event6(new_shared_objects, nullptr,
                                          empty_return_value);

  unordered_map<SharedObject*, PeerObjectImpl*> new_peer_objects;

  PeerThread peer_thread;
  peer_thread.Start(&transaction_store, &shared_object1, live_object1,
                    &new_peer_objects);

  peer_thread.QueueEvent(&event1);
  peer_thread.QueueEvent(&event2);

  peer_thread.FlushEvents();

  peer_thread.QueueEvent(&event3);
  peer_thread.QueueEvent(&event4);
  peer_thread.QueueEvent(&event5);

  peer_thread.FlushEvents();

  peer_thread.QueueEvent(&event6);

  peer_thread.Stop();

  EXPECT_FALSE(peer_thread.conflict_detected());
}

void TestMethod5(Thread* thread, const vector<Value>& parameters,
                 Value* return_value) {
  CHECK(thread != nullptr);
  CHECK_EQ(parameters.size(), 0u);
  CHECK(return_value != nullptr);

  PeerObject* const peer_object = thread->CreatePeerObject(
      new FakeLocalObject(""), "", true);

  {
    Value sub_return_value;
    if (!thread->CallMethod(peer_object, "test_method6", vector<Value>(),
                            &sub_return_value)) {
      return;
    }
  }

  {
    Value sub_return_value;
    if (!thread->CallMethod(peer_object, "test_method7", vector<Value>(),
                            &sub_return_value)) {
      return;
    }
  }

  return_value->set_empty(0);
}

TEST(PeerThreadTest, NewObjectIsUsedInTwoEvents) {
  MockTransactionStoreCore transaction_store_core;
  MockTransactionStore transaction_store(&transaction_store_core);
  SharedObject shared_object1(&transaction_store, MakeUuid(1));
  SharedObject shared_object2(&transaction_store, MakeUuid(2));
  const MockLocalObjectCore local_object_core1;
  LiveObjectPtr live_object1(
      new LiveObject(new MockLocalObject(&local_object_core1)));

  EXPECT_CALL(transaction_store_core, GetCurrentSequencePoint())
      .Times(0);
  EXPECT_CALL(transaction_store_core, GetLiveObjectAtSequencePoint(_, _, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, CreateUnboundPeerObject(_))
      .Times(AnyNumber());
  EXPECT_CALL(transaction_store_core, CreateBoundPeerObject(_, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, CreateTransaction(_, _, _, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, ObjectsAreEquivalent(_, _))
      .Times(0);

  EXPECT_CALL(local_object_core1, Serialize(_))
      .Times(0);
  EXPECT_CALL(local_object_core1, InvokeMethod(_, _, "test_method5", _, _))
      .WillOnce(WithArgs<0, 3, 4>(Invoke(&TestMethod5)));

  unordered_set<SharedObject*> new_shared_objects;
  new_shared_objects.insert(&shared_object2);

  const vector<CommittedValue> method_parameters;

  CommittedValue empty_return_value;
  empty_return_value.set_local_type(0);
  empty_return_value.set_empty();

  const MethodCallCommittedEvent event1(nullptr, "test_method5",
                                        method_parameters);
  const SubMethodCallCommittedEvent event2(new_shared_objects, &shared_object2,
                                           "test_method6",
                                           vector<CommittedValue>());
  const SubMethodReturnCommittedEvent event3(&shared_object2,
                                             empty_return_value);
  const SubMethodCallCommittedEvent event4(unordered_set<SharedObject*>(),
                                           &shared_object2, "test_method7",
                                           vector<CommittedValue>());
  const SubMethodReturnCommittedEvent event5(&shared_object2,
                                             empty_return_value);
  const MethodReturnCommittedEvent event6(unordered_set<SharedObject*>(),
                                          nullptr, empty_return_value);

  unordered_map<SharedObject*, PeerObjectImpl*> new_peer_objects;

  PeerThread peer_thread;
  peer_thread.Start(&transaction_store, &shared_object1, live_object1,
                    &new_peer_objects);

  peer_thread.QueueEvent(&event1);
  peer_thread.QueueEvent(&event2);
  peer_thread.QueueEvent(&event3);
  peer_thread.QueueEvent(&event4);
  peer_thread.QueueEvent(&event5);
  peer_thread.QueueEvent(&event6);

  peer_thread.Stop();

  EXPECT_FALSE(peer_thread.conflict_detected());

  EXPECT_EQ(1, new_peer_objects.size());
  EXPECT_FALSE(new_peer_objects.find(&shared_object2) ==
               new_peer_objects.end());
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
