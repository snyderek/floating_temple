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

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <gflags/gflags.h>

#include "base/logging.h"
#include "engine/live_object.h"
#include "engine/mock_local_object.h"
#include "engine/mock_sequence_point.h"
#include "engine/mock_transaction_store.h"
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
using std::size_t;
using std::string;
using std::vector;
using testing::AnyNumber;
using testing::AtLeast;
using testing::ElementsAre;
using testing::InSequence;
using testing::InitGoogleMock;
using testing::Return;
using testing::ReturnNew;
using testing::_;

namespace floating_temple {

class ObjectReference;
class ObjectReferenceImpl;

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

class TestLocalObject : public LocalObject {
 public:
  size_t Serialize(void* buffer, size_t buffer_size,
                   SerializationContext* context) const override {
    LOG(FATAL) << "Not implemented.";
    return 0;
  }

  void Dump(DumpContext* dc) const override {
    LOG(FATAL) << "Not implemented.";
  }
};

//------------------------------------------------------------------------------

class CallMethodInNestedTransactions_ProgramObject : public TestLocalObject {
 public:
  LocalObject* Clone() const override {
    return new CallMethodInNestedTransactions_ProgramObject();
  }

  void InvokeMethod(Thread* thread,
                    ObjectReference* object_reference,
                    const string& method_name,
                    const vector<Value>& parameters,
                    Value* return_value) override {
    LocalObject* const fake_local_object = new FakeLocalObject("a");
    ObjectReference* const fake_local_object_reference = thread->CreateObject(
        fake_local_object, "");

    CHECK(thread->BeginTransaction());
    CallAppendMethod(thread, fake_local_object_reference, "b");
    CHECK(thread->BeginTransaction());
    CallAppendMethod(thread, fake_local_object_reference, "c");
    CHECK(thread->EndTransaction());
    CallAppendMethod(thread, fake_local_object_reference, "d");
    CHECK(thread->EndTransaction());
  }
};

TEST(RecordingThreadTest, CallMethodInNestedTransactions) {
  MockTransactionStoreCore transaction_store_core;
  MockTransactionStore transaction_store(&transaction_store_core);

  EXPECT_CALL(transaction_store_core, GetCurrentSequencePoint())
      .WillRepeatedly(ReturnNew<MockSequencePoint>());
  EXPECT_CALL(transaction_store_core, CreateUnboundObjectReference())
      .Times(AnyNumber());

  {
    InSequence s;

    EXPECT_CALL(
        transaction_store_core,
        CreateTransaction(ElementsAre(IsMethodCallPendingEvent("run"),
                                      IsBeginTransactionPendingEvent()),
                          _, _, _));

    EXPECT_CALL(
        transaction_store_core,
        CreateTransaction(ElementsAre(IsMethodCallPendingEvent("append"),
                                      IsMethodReturnPendingEvent(),
                                      IsBeginTransactionPendingEvent(),
                                      IsMethodCallPendingEvent("append"),
                                      IsMethodReturnPendingEvent(),
                                      IsEndTransactionPendingEvent(),
                                      IsMethodCallPendingEvent("append"),
                                      IsMethodReturnPendingEvent(),
                                      IsEndTransactionPendingEvent()),
                          _, _, _));

    EXPECT_CALL(
        transaction_store_core,
        CreateTransaction(ElementsAre(IsMethodReturnPendingEvent()), _, _, _));
  }

  RecordingThread recording_thread(&transaction_store);
  LocalObject* const program_object =
      new CallMethodInNestedTransactions_ProgramObject();

  Value return_value;
  recording_thread.RunProgram(program_object, "run", &return_value, false);
}

//------------------------------------------------------------------------------

class CallBeginTransactionFromWithinMethod_FakeLocalObject
    : public TestLocalObject {
 public:
  LocalObject* Clone() const override {
    return new CallBeginTransactionFromWithinMethod_FakeLocalObject();
  }

  void InvokeMethod(Thread* thread,
                    ObjectReference* object_reference,
                    const string& method_name,
                    const vector<Value>& parameters,
                    Value* return_value) override {
    CHECK(thread->BeginTransaction());

    ObjectReference* const new_object_reference = thread->CreateObject(
        new MockLocalObject(&mock_local_object_core_), "");
    return_value->set_object_reference(0, new_object_reference);
  }

 private:
  MockLocalObjectCore mock_local_object_core_;
};

class CallBeginTransactionFromWithinMethod_ProgramObject
    : public TestLocalObject {
 public:
  LocalObject* Clone() const override {
    return new CallBeginTransactionFromWithinMethod_ProgramObject();
  }

  void InvokeMethod(Thread* thread,
                    ObjectReference* object_reference,
                    const string& method_name,
                    const vector<Value>& parameters,
                    Value* return_value) override {
    ObjectReference* const fake_local_object_reference = thread->CreateObject(
        new CallBeginTransactionFromWithinMethod_FakeLocalObject(), "");

    Value method_return_value;
    CHECK(thread->CallMethod(fake_local_object_reference, "test-method",
                             vector<Value>(), &method_return_value));
  }
};

TEST(RecordingThreadTest, CallBeginTransactionFromWithinMethod) {
  MockTransactionStoreCore transaction_store_core;
  MockTransactionStore transaction_store(&transaction_store_core);

  const shared_ptr<const LiveObject> fake_live_object(
      new LiveObject(
          new CallBeginTransactionFromWithinMethod_FakeLocalObject()));

  EXPECT_CALL(transaction_store_core, GetCurrentSequencePoint())
      .WillRepeatedly(ReturnNew<MockSequencePoint>());
  EXPECT_CALL(transaction_store_core, GetLiveObjectAtSequencePoint(_, _, _))
      .WillRepeatedly(Return(fake_live_object));
  EXPECT_CALL(transaction_store_core, CreateUnboundObjectReference())
      .Times(AnyNumber());

  {
    InSequence s;

    EXPECT_CALL(
        transaction_store_core,
        CreateTransaction(ElementsAre(IsMethodCallPendingEvent("run"),
                                      IsMethodCallPendingEvent("test-method")),
                          _, _, _));

    EXPECT_CALL(
        transaction_store_core,
        CreateTransaction(ElementsAre(IsBeginTransactionPendingEvent()),
                          _, _, _));
  }

  RecordingThread recording_thread(&transaction_store);
  LocalObject* const program_object =
      new CallBeginTransactionFromWithinMethod_ProgramObject();

  // Run the program, which creates a fake local object and calls the
  // "test-method" method on it. That method calls Thread::BeginTransaction,
  // creates a new object, and returns the new object reference. The
  // RecordingThread instance should create two implicit transactions:
  //
  // The first transaction should contain the start of the "run" call, up to the
  // call to "test-method".
  //
  // The second transaction should contain the start of "test-method", up to the
  // call to BeginTransaction.
  //
  // No other transaction should be created, because the explicit transaction
  // (initiated by the call to BeginTransaction) is never terminated.

  Value return_value;
  recording_thread.RunProgram(program_object, "run", &return_value, false);
}

//------------------------------------------------------------------------------

class CallEndTransactionFromWithinMethod_FakeLocalObject
    : public TestLocalObject {
 public:
  LocalObject* Clone() const override {
    return new CallEndTransactionFromWithinMethod_FakeLocalObject();
  }

  void InvokeMethod(Thread* thread,
                    ObjectReference* object_reference,
                    const string& method_name,
                    const vector<Value>& parameters,
                    Value* return_value) override {
    CHECK(thread->EndTransaction());

    ObjectReference* const new_object_reference = thread->CreateObject(
        new MockLocalObject(&mock_local_object_core_), "");
    return_value->set_object_reference(0, new_object_reference);
  }

 private:
  MockLocalObjectCore mock_local_object_core_;
};

class CallEndTransactionFromWithinMethod_ProgramObject
    : public TestLocalObject {
 public:
  LocalObject* Clone() const override {
    return new CallEndTransactionFromWithinMethod_ProgramObject();
  }

  void InvokeMethod(Thread* thread,
                    ObjectReference* object_reference,
                    const string& method_name,
                    const vector<Value>& parameters,
                    Value* return_value) override {
  // Start an explicit transaction.
    CHECK(thread->BeginTransaction());

    ObjectReference* const fake_local_object_reference = thread->CreateObject(
        new CallEndTransactionFromWithinMethod_FakeLocalObject(), "");

    Value method_return_value;
    CHECK(thread->CallMethod(fake_local_object_reference, "test-method",
                             vector<Value>(), &method_return_value));
  }
};

TEST(RecordingThreadTest, CallEndTransactionFromWithinMethod) {
  MockTransactionStoreCore transaction_store_core;
  MockTransactionStore transaction_store(&transaction_store_core);

  const shared_ptr<const LiveObject> fake_live_object(
      new LiveObject(new CallEndTransactionFromWithinMethod_FakeLocalObject()));

  EXPECT_CALL(transaction_store_core, GetCurrentSequencePoint())
      .WillRepeatedly(ReturnNew<MockSequencePoint>());
  EXPECT_CALL(transaction_store_core, GetLiveObjectAtSequencePoint(_, _, _))
      .WillRepeatedly(Return(fake_live_object));
  EXPECT_CALL(transaction_store_core, CreateUnboundObjectReference())
      .Times(AnyNumber());

  {
    InSequence s;

    EXPECT_CALL(
        transaction_store_core,
        CreateTransaction(ElementsAre(IsMethodCallPendingEvent("run"),
                                      IsBeginTransactionPendingEvent()),
                          _, _, _));

    EXPECT_CALL(
        transaction_store_core,
        CreateTransaction(ElementsAre(IsMethodCallPendingEvent("test-method"),
                                      IsEndTransactionPendingEvent()),
                          _, _, _));

    EXPECT_CALL(
        transaction_store_core,
        CreateTransaction(ElementsAre(IsMethodReturnPendingEvent()), _, _, _))
        .Times(2);
  }

  RecordingThread recording_thread(&transaction_store);
  LocalObject* const program_object =
      new CallEndTransactionFromWithinMethod_ProgramObject();

  // Run the program, which begins an explicit transaction, creates a fake local
  // object, and calls the "test-method" method on the object. That method calls
  // Thread::EndTransaction, creates a new object, and returns the new object
  // reference. The RecordingThread instance should create four transactions:
  //
  // The first transaction (implicit) should contain the start of the "run"
  // call, up to the call to BeginTransaction.
  //
  // The second transaction (explicit) should contain everything from the
  // BeginTransaction call to the EndTransaction call.
  //
  // The third transaction (implicit) should contain the end of the
  // "test-method" call.
  //
  // The fourth transaction (implicit) should contain the end of the "run" call.

  Value return_value;
  recording_thread.RunProgram(program_object, "run", &return_value, false);
}

//------------------------------------------------------------------------------

class CreateObjectInDifferentTransaction_ProgramObject
    : public TestLocalObject {
 public:
  LocalObject* Clone() const override {
    return new CreateObjectInDifferentTransaction_ProgramObject();
  }

  void InvokeMethod(Thread* thread,
                    ObjectReference* object_reference,
                    const string& method_name,
                    const vector<Value>& parameters,
                    Value* return_value) override {
    // Create an object, and then call a method on that object in a different
    // transaction. The object should still be available in the later
    // transaction, even though the content of the object was never committed.
    // (An object is not committed until it's involved in a method call.)

    CHECK(thread->BeginTransaction());
    ObjectReference* const object_reference1 = thread->CreateObject(
        new FakeLocalObject("lucy."), "");
    ObjectReference* const object_reference2 = thread->CreateObject(
        new FakeLocalObject("ethel."), "");
    // This method call is here only to force a transaction to be created.
    CallAppendMethod(thread, object_reference1, "ricky.");
    CHECK(thread->EndTransaction());

    CHECK(thread->BeginTransaction());
    // object_reference2 should still be available, even though it was created
    // in an earlier transaction.
    CallAppendMethod(thread, object_reference2, "fred.");
    CHECK(thread->EndTransaction());
  }
};

TEST(RecordingThreadTest, CreateObjectInDifferentTransaction) {
  MockTransactionStoreCore transaction_store_core;
  MockTransactionStore transaction_store(&transaction_store_core);

  EXPECT_CALL(transaction_store_core, GetCurrentSequencePoint())
      .WillRepeatedly(ReturnNew<MockSequencePoint>());
  // TransactionStoreInternalInterface::GetLiveObjectAtSequencePoint should not
  // be called, because the thread already has a copy of the object (the only
  // copy, in fact, since the object hasn't been committed).
  EXPECT_CALL(transaction_store_core, GetLiveObjectAtSequencePoint(_, _, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, CreateUnboundObjectReference())
      .Times(AnyNumber());

  EXPECT_CALL(transaction_store_core, CreateTransaction(_, _, _, _))
      .Times(AtLeast(2));

  RecordingThread recording_thread(&transaction_store);
  LocalObject* const program_object =
      new CreateObjectInDifferentTransaction_ProgramObject();

  Value return_value;
  recording_thread.RunProgram(program_object, "run", &return_value, false);
}

//------------------------------------------------------------------------------

}  // namespace
}  // namespace engine
}  // namespace floating_temple

int main(int argc, char** argv) {
  ParseCommandLineFlags(&argc, &argv, true);
  InitGoogleLogging(argv[0]);
  InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
