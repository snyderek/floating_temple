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

#include "engine/recording_thread.h"

#include <memory>
#include <string>
#include <vector>

#include <gflags/gflags.h>

#include "base/logging.h"
#include "base/macros.h"
#include "engine/live_object.h"
#include "engine/mock_local_object.h"
#include "engine/mock_sequence_point.h"
#include "engine/mock_transaction_store.h"
#include "engine/object_reference_impl.h"
#include "engine/pending_event.h"
#include "fake_interpreter/fake_local_object.h"
#include "include/c++/local_object.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "third_party/gmock-1.7.0/gtest/include/gtest/gtest.h"
#include "third_party/gmock-1.7.0/include/gmock/gmock.h"

using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
using std::shared_ptr;
using std::string;
using std::vector;
using testing::AnyNumber;
using testing::AtLeast;
using testing::DoAll;
using testing::ElementsAre;
using testing::InSequence;
using testing::InitGoogleMock;
using testing::Invoke;
using testing::IsEmpty;
using testing::Return;
using testing::ReturnNew;
using testing::WithArg;
using testing::_;

namespace floating_temple {

class ObjectReference;

namespace engine {
namespace {

MATCHER(IsBeginTransactionPendingEvent, "") {
  return arg->type() == PendingEvent::BEGIN_TRANSACTION;
}

MATCHER(IsEndTransactionPendingEvent, "") {
  return arg->type() == PendingEvent::END_TRANSACTION;
}

MATCHER_P(IsMethodCallPendingEvent, expected_method_name, "") {
  if (arg->type() != PendingEvent::METHOD_CALL) {
    return false;
  }

  ObjectReferenceImpl* next_object_reference = nullptr;
  const string* method_name = nullptr;
  const vector<Value>* parameters = nullptr;

  arg->GetMethodCall(&next_object_reference, &method_name, &parameters);

  return *method_name == expected_method_name;
}

MATCHER(IsMethodReturnPendingEvent, "") {
  return arg->type() == PendingEvent::METHOD_RETURN;
}

void CallAppendMethod(Thread* thread, ObjectReference* object_reference,
                      const string& string_to_append) {
  CHECK(thread != nullptr);

  vector<Value> parameters(1);
  parameters[0].set_string_value(FakeLocalObject::kStringLocalType,
                                 string_to_append);

  Value return_value;
  CHECK(thread->CallMethod(object_reference, "append", parameters,
                           &return_value));

  CHECK_EQ(return_value.local_type(), FakeLocalObject::kVoidLocalType);
  CHECK_EQ(return_value.type(), Value::EMPTY);
}

class ValueSetter {
 public:
  explicit ValueSetter(const Value& desired_value)
      : desired_value_(desired_value) {
  }

  void SetValue(Value* value) const {
    CHECK(value != nullptr);
    *value = desired_value_;
  }

 private:
  const Value desired_value_;

  DISALLOW_COPY_AND_ASSIGN(ValueSetter);
};

TEST(RecordingThreadTest, CallMethodInNestedTransactions) {
  ObjectReferenceImpl object_reference;
  MockTransactionStoreCore transaction_store_core;
  MockTransactionStore transaction_store(&transaction_store_core);
  RecordingThread thread(&transaction_store);
  const shared_ptr<const LiveObject> initial_live_object(
      new LiveObject(new FakeLocalObject("a")));

  EXPECT_CALL(transaction_store_core, GetCurrentSequencePoint())
      .WillRepeatedly(ReturnNew<MockSequencePoint>());
  EXPECT_CALL(transaction_store_core,
              GetLiveObjectAtSequencePoint(&object_reference, _, _))
      .WillRepeatedly(Return(initial_live_object));
  // TODO(dss): Add expectations for
  // transaction_store_core.CreateUnboundObjectReference and
  // transaction_store_core.CreateBoundObjectReference.
  EXPECT_CALL(transaction_store_core, ObjectsAreIdentical(_, _))
      .Times(0);

  EXPECT_CALL(transaction_store_core, CreateTransaction(_, _, _, _));

  ASSERT_TRUE(thread.BeginTransaction());
  CallAppendMethod(&thread, &object_reference, "b");
  ASSERT_TRUE(thread.BeginTransaction());
  CallAppendMethod(&thread, &object_reference, "c");
  ASSERT_TRUE(thread.EndTransaction());
  CallAppendMethod(&thread, &object_reference, "d");
  ASSERT_TRUE(thread.EndTransaction());
}

void CallBeginTransaction(Thread* thread) {
  CHECK(thread != nullptr);
  CHECK(thread->BeginTransaction());
}

void CallEndTransaction(Thread* thread) {
  CHECK(thread != nullptr);
  CHECK(thread->EndTransaction());
}

TEST(RecordingThreadTest, CallBeginTransactionFromWithinMethod) {
  MockTransactionStoreCore transaction_store_core;
  MockTransactionStore transaction_store(&transaction_store_core);
  ObjectReferenceImpl object_reference, new_object_reference;
  const MockLocalObjectCore local_object_core;
  shared_ptr<const LiveObject> live_object(
      new LiveObject(new MockLocalObject(&local_object_core)));

  RecordingThread thread(&transaction_store);

  EXPECT_CALL(transaction_store_core, GetCurrentSequencePoint())
      .WillRepeatedly(ReturnNew<MockSequencePoint>());
  EXPECT_CALL(transaction_store_core,
              GetLiveObjectAtSequencePoint(&object_reference, _, _))
      .WillRepeatedly(Return(live_object));
  EXPECT_CALL(transaction_store_core, CreateUnboundObjectReference())
      .Times(AnyNumber());
  EXPECT_CALL(transaction_store_core, CreateBoundObjectReference(_))
      .Times(0);
  EXPECT_CALL(transaction_store_core, ObjectsAreIdentical(_, _))
      .Times(0);

  Value canned_return_value;
  canned_return_value.set_object_reference(0, &new_object_reference);
  const ValueSetter value_setter(canned_return_value);

  EXPECT_CALL(local_object_core, Serialize(_))
      .Times(0);
  EXPECT_CALL(local_object_core,
              InvokeMethod(_, _, "test-method", IsEmpty(), _))
      .WillRepeatedly(DoAll(WithArg<0>(Invoke(&CallBeginTransaction)),
                            WithArg<4>(Invoke(&value_setter,
                                              &ValueSetter::SetValue))));

  // The implicit transaction that should be created.
  EXPECT_CALL(
      transaction_store_core,
      CreateTransaction(ElementsAre(IsMethodCallPendingEvent("test-method"),
                                    IsBeginTransactionPendingEvent()),
                        _, _, _));

  // Call the "test-method" method. The method calls Thread::BeginTransaction,
  // creates a new object, and returns the new object reference. The
  // RecordingThread instance should create an implicit transaction that
  // contains the start of the "test-method" call and the call to
  // BeginTransaction.
  //
  // No other transaction should be created, because the explicit transaction
  // (initiated by the call to BeginTransaction) is never terminated.

  Value return_value;
  ASSERT_TRUE(thread.CallMethod(&object_reference, "test-method",
                                vector<Value>(), &return_value));
  EXPECT_EQ(&new_object_reference, return_value.object_reference());
}

TEST(RecordingThreadTest, CallEndTransactionFromWithinMethod) {
  MockTransactionStoreCore transaction_store_core;
  MockTransactionStore transaction_store(&transaction_store_core);
  ObjectReferenceImpl object_reference;
  const MockLocalObjectCore local_object_core;
  shared_ptr<const LiveObject> live_object(
      new LiveObject(new MockLocalObject(&local_object_core)));

  RecordingThread thread(&transaction_store);

  EXPECT_CALL(transaction_store_core, GetCurrentSequencePoint())
      .WillRepeatedly(ReturnNew<MockSequencePoint>());
  EXPECT_CALL(transaction_store_core,
              GetLiveObjectAtSequencePoint(&object_reference, _, _))
      .WillRepeatedly(Return(live_object));
  EXPECT_CALL(transaction_store_core, CreateUnboundObjectReference())
      .Times(AnyNumber());
  EXPECT_CALL(transaction_store_core, CreateBoundObjectReference(_))
      .Times(0);
  EXPECT_CALL(transaction_store_core, ObjectsAreIdentical(_, _))
      .Times(0);

  Value canned_return_value;
  canned_return_value.set_empty(0);
  const ValueSetter value_setter(canned_return_value);

  EXPECT_CALL(local_object_core, Serialize(_))
      .Times(0);
  EXPECT_CALL(local_object_core,
              InvokeMethod(_, _, "test-method", IsEmpty(), _))
      .WillRepeatedly(DoAll(WithArg<0>(Invoke(&CallEndTransaction)),
                            WithArg<4>(Invoke(&value_setter,
                                              &ValueSetter::SetValue))));

  {
    InSequence s;

    // The explicit transaction that should be created.
    EXPECT_CALL(
        transaction_store_core,
        CreateTransaction(ElementsAre(IsMethodCallPendingEvent("test-method"),
                                      IsEndTransactionPendingEvent()),
                          _, _, _));

    // The implicit transaction that should be created.
    EXPECT_CALL(
        transaction_store_core,
        CreateTransaction(ElementsAre(IsMethodReturnPendingEvent()), _, _, _));
  }

  // Start an explicit transaction.
  ASSERT_TRUE(thread.BeginTransaction());

  // Call the "test-method" method. The method calls Thread::EndTransaction and
  // then returns. The RecordingThread instance should create two transactions:
  //
  // The first transaction (explicit) contains everything from the
  // BeginTransaction call to the EndTransaction call.
  //
  // The second transaction (implicit) contains everything from the
  // EndTransaction call to the "test-method" return.

  Value return_value;
  ASSERT_TRUE(thread.CallMethod(&object_reference, "test-method",
                                vector<Value>(), &return_value));
  EXPECT_EQ(Value::EMPTY, return_value.type());
}

// Create an object, and then call a method on that object in a different
// transaction. The object should still be available in the later transaction,
// even though the content of the object was never committed. (An object is not
// committed until it's involved in a method call.)
TEST(RecordingThreadTest, CreateObjectInDifferentTransaction) {
  MockTransactionStoreCore transaction_store_core;
  MockTransactionStore transaction_store(&transaction_store_core);
  RecordingThread thread(&transaction_store);

  EXPECT_CALL(transaction_store_core, GetCurrentSequencePoint())
      .WillRepeatedly(ReturnNew<MockSequencePoint>());
  // TransactionStoreInternalInterface::GetLiveObjectAtSequencePoint should not
  // be called, because the thread already has a copy of the object (the only
  // copy, in fact, since the object hasn't been committed).
  EXPECT_CALL(transaction_store_core, GetLiveObjectAtSequencePoint(_, _, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, CreateUnboundObjectReference())
      .Times(AtLeast(1));
  EXPECT_CALL(transaction_store_core, CreateBoundObjectReference(_))
      .Times(0);
  EXPECT_CALL(transaction_store_core, ObjectsAreIdentical(_, _))
      .Times(0);

  EXPECT_CALL(transaction_store_core, CreateTransaction(_, _, _, _))
      .Times(2);

  ASSERT_TRUE(thread.BeginTransaction());
  ObjectReference* const object_reference1 = thread.CreateObject(
      new FakeLocalObject("lucy."), "");
  ObjectReference* const object_reference2 = thread.CreateObject(
      new FakeLocalObject("ethel."), "");
  // This method call is here only to force a transaction to be created.
  CallAppendMethod(&thread, object_reference1, "ricky.");
  ASSERT_TRUE(thread.EndTransaction());

  ASSERT_TRUE(thread.BeginTransaction());
  // object_reference2 should still be available, even though it was created in
  // an earlier transaction.
  CallAppendMethod(&thread, object_reference2, "fred.");
  ASSERT_TRUE(thread.EndTransaction());
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
