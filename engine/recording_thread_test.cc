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
#include <string>
#include <vector>

#include <gflags/gflags.h>

#include "base/escape.h"
#include "base/logging.h"
#include "engine/canonical_peer.h"
#include "engine/committed_event.h"
#include "engine/mock_local_object.h"
#include "engine/mock_transaction_store.h"
#include "engine/proto/transaction_id.pb.h"
#include "engine/shared_object_transaction.h"
#include "engine/transaction_store_internal_interface.h"
#include "fake_interpreter/fake_local_object.h"
#include "include/c++/local_object.h"
#include "include/c++/method_context.h"
#include "include/c++/value.h"
#include "third_party/gmock-1.7.0/gtest/include/gtest/gtest.h"
#include "third_party/gmock-1.7.0/include/gmock/gmock.h"

using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
using std::size_t;
using std::string;
using std::vector;
using testing::AtLeast;
using testing::ElementsAre;
using testing::InSequence;
using testing::InitGoogleMock;
using testing::Pair;
using testing::Pointee;
using testing::Return;
using testing::Sequence;
using testing::UnorderedElementsAre;
using testing::_;

namespace floating_temple {

class ObjectReference;
class ObjectReferenceImpl;

namespace engine {
namespace {

MATCHER(IsObjectCreationEvent, "") {
  return arg->type() == CommittedEvent::OBJECT_CREATION;
}

MATCHER(IsBeginTransactionEvent, "") {
  return arg->type() == CommittedEvent::BEGIN_TRANSACTION;
}

MATCHER(IsEndTransactionEvent, "") {
  return arg->type() == CommittedEvent::END_TRANSACTION;
}

MATCHER_P(IsMethodCallEvent, expected_method_name, "") {
  if (arg->type() != CommittedEvent::METHOD_CALL) {
    return false;
  }

  const string* method_name = nullptr;
  const vector<Value>* parameters = nullptr;

  arg->GetMethodCall(&method_name, &parameters);

  return *method_name == expected_method_name;
}

MATCHER(IsMethodReturnEvent, "") {
  return arg->type() == CommittedEvent::METHOD_RETURN;
}

MATCHER_P(IsSubMethodCallEvent, expected_method_name, "") {
  if (arg->type() != CommittedEvent::SUB_METHOD_CALL) {
    return false;
  }

  ObjectReferenceImpl* callee = nullptr;
  const string* method_name = nullptr;
  const vector<Value>* parameters = nullptr;

  arg->GetSubMethodCall(&callee, &method_name, &parameters);

  return *method_name == expected_method_name;
}

MATCHER(IsSubMethodReturnEvent, "") {
  return arg->type() == CommittedEvent::SUB_METHOD_RETURN;
}

MATCHER_P(IsSelfMethodCallEvent, expected_method_name, "") {
  if (arg->type() != CommittedEvent::SELF_METHOD_CALL) {
    return false;
  }

  const string* method_name = nullptr;
  const vector<Value>* parameters = nullptr;

  arg->GetSelfMethodCall(&method_name, &parameters);

  return *method_name == expected_method_name;
}

MATCHER(IsSelfMethodReturnEvent, "") {
  return arg->type() == CommittedEvent::SELF_METHOD_RETURN;
}

void CallAppendMethod(MethodContext* method_context,
                      ObjectReference* object_reference,
                      const string& string_to_append) {
  CHECK(method_context != nullptr);

  vector<Value> parameters(1);
  parameters[0].set_string_value(FakeLocalObject::kStringLocalType,
                                 string_to_append);

  Value return_value;
  CHECK(method_context->CallMethod(object_reference, "append", parameters,
                                   &return_value));

  CHECK_EQ(return_value.local_type(), FakeLocalObject::kVoidLocalType);
  CHECK_EQ(return_value.type(), Value::EMPTY);
}

class TestLocalObject : public LocalObject {
 public:
  size_t Serialize(void* buffer, size_t buffer_size,
                   SerializationContext* context) const override {
    LOG(FATAL) << "Not implemented.";
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

  void InvokeMethod(MethodContext* method_context,
                    ObjectReference* self_object_reference,
                    const string& method_name,
                    const vector<Value>& parameters,
                    Value* return_value) override {
    CHECK_EQ(method_name, "run");

    LocalObject* const fake_local_object = new FakeLocalObject("a");
    ObjectReference* const fake_local_object_reference =
        method_context->CreateObject(fake_local_object, "");

    CHECK(method_context->BeginTransaction());
    CallAppendMethod(method_context, fake_local_object_reference, "b");
    CHECK(method_context->BeginTransaction());
    CallAppendMethod(method_context, fake_local_object_reference, "c");
    CHECK(method_context->EndTransaction());
    CallAppendMethod(method_context, fake_local_object_reference, "d");
    CHECK(method_context->EndTransaction());
  }
};

TEST(RecordingThreadTest, CallMethodInNestedTransactions) {
  CanonicalPeer fake_local_peer("test-local-peer");
  MockTransactionStoreCore transaction_store_core;
  MockTransactionStore transaction_store(&transaction_store_core);

  EXPECT_CALL(transaction_store_core, GetLocalPeer())
      .WillRepeatedly(Return(&fake_local_peer));
  EXPECT_CALL(transaction_store_core, GetExecutionPhase(_))
      .WillRepeatedly(Return(TransactionStoreInternalInterface::NORMAL));

  {
    InSequence s;

    EXPECT_CALL(
        transaction_store_core,
        CreateTransaction(UnorderedElementsAre(
            Pair(_, Pointee(ElementsAre(
                IsObjectCreationEvent(),
                IsMethodCallEvent("run"),
                IsBeginTransactionEvent())))),
            _, _, _));

    EXPECT_CALL(
        transaction_store_core,
        CreateTransaction(UnorderedElementsAre(
            Pair(_, Pointee(ElementsAre(
                IsSubMethodCallEvent("append"),
                IsSubMethodReturnEvent(),
                IsBeginTransactionEvent(),
                IsSubMethodCallEvent("append"),
                IsSubMethodReturnEvent(),
                IsEndTransactionEvent(),
                IsSubMethodCallEvent("append"),
                IsSubMethodReturnEvent(),
                IsEndTransactionEvent()))),
            Pair(_, Pointee(ElementsAre(
                IsObjectCreationEvent(),
                IsMethodCallEvent("append"),
                IsMethodReturnEvent(),
                IsMethodCallEvent("append"),
                IsMethodReturnEvent(),
                IsMethodCallEvent("append"),
                IsMethodReturnEvent())))),
            _, _, _));

    EXPECT_CALL(
        transaction_store_core,
        CreateTransaction(UnorderedElementsAre(
            Pair(_, Pointee(ElementsAre(
                IsMethodReturnEvent())))),
            _, _, _));
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

  void InvokeMethod(MethodContext* method_context,
                    ObjectReference* self_object_reference,
                    const string& method_name,
                    const vector<Value>& parameters,
                    Value* return_value) override {
    CHECK_EQ(method_name, "test-method");

    CHECK(method_context->BeginTransaction());

    ObjectReference* const new_object_reference = method_context->CreateObject(
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

  void InvokeMethod(MethodContext* method_context,
                    ObjectReference* object_reference,
                    const string& method_name,
                    const vector<Value>& parameters,
                    Value* return_value) override {
    CHECK_EQ(method_name, "run");

    ObjectReference* const fake_local_object_reference =
        method_context->CreateObject(
            new CallBeginTransactionFromWithinMethod_FakeLocalObject(), "");

    Value method_return_value;
    CHECK(method_context->CallMethod(fake_local_object_reference, "test-method",
                                     vector<Value>(), &method_return_value));
  }
};

TEST(RecordingThreadTest, CallBeginTransactionFromWithinMethod) {
  CanonicalPeer fake_local_peer("test-local-peer");
  MockTransactionStoreCore transaction_store_core;
  MockTransactionStore transaction_store(&transaction_store_core);

  EXPECT_CALL(transaction_store_core, GetLocalPeer())
      .WillRepeatedly(Return(&fake_local_peer));
  EXPECT_CALL(transaction_store_core, GetExecutionPhase(_))
      .WillRepeatedly(Return(TransactionStoreInternalInterface::NORMAL));

  {
    InSequence s;

    EXPECT_CALL(
        transaction_store_core,
        CreateTransaction(UnorderedElementsAre(
            Pair(_, Pointee(ElementsAre(
                IsObjectCreationEvent(),
                IsMethodCallEvent("run"),
                IsSubMethodCallEvent("test-method")))),
            Pair(_, Pointee(ElementsAre(
                IsObjectCreationEvent(),
                IsMethodCallEvent("test-method"))))),
            _, _, _));

    EXPECT_CALL(
        transaction_store_core,
        CreateTransaction(UnorderedElementsAre(
            Pair(_, Pointee(ElementsAre(
                IsBeginTransactionEvent())))),
            _, _, _));
  }

  RecordingThread recording_thread(&transaction_store);
  LocalObject* const program_object =
      new CallBeginTransactionFromWithinMethod_ProgramObject();

  // Run the program, which creates a fake local object and calls the
  // "test-method" method on it. That method calls
  // MethodContext::BeginTransaction, creates a new object, and returns the new
  // object reference. The RecordingThread instance should create two implicit
  // transactions:
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

  void InvokeMethod(MethodContext* method_context,
                    ObjectReference* self_object_reference,
                    const string& method_name,
                    const vector<Value>& parameters,
                    Value* return_value) override {
    CHECK_EQ(method_name, "test-method");

    CHECK(method_context->EndTransaction());

    ObjectReference* const new_object_reference = method_context->CreateObject(
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

  void InvokeMethod(MethodContext* method_context,
                    ObjectReference* self_object_reference,
                    const string& method_name,
                    const vector<Value>& parameters,
                    Value* return_value) override {
    CHECK_EQ(method_name, "run");

    // Start an explicit transaction.
    CHECK(method_context->BeginTransaction());

    ObjectReference* const fake_local_object_reference =
        method_context->CreateObject(
            new CallEndTransactionFromWithinMethod_FakeLocalObject(), "");

    Value method_return_value;
    CHECK(method_context->CallMethod(fake_local_object_reference, "test-method",
                                     vector<Value>(), &method_return_value));
  }
};

TEST(RecordingThreadTest, CallEndTransactionFromWithinMethod) {
  CanonicalPeer fake_local_peer("test-local-peer");
  MockTransactionStoreCore transaction_store_core;
  MockTransactionStore transaction_store(&transaction_store_core);

  EXPECT_CALL(transaction_store_core, GetLocalPeer())
      .WillRepeatedly(Return(&fake_local_peer));
  EXPECT_CALL(transaction_store_core, GetExecutionPhase(_))
      .WillRepeatedly(Return(TransactionStoreInternalInterface::NORMAL));

  {
    InSequence s;

    EXPECT_CALL(
        transaction_store_core,
        CreateTransaction(UnorderedElementsAre(
            Pair(_, Pointee(ElementsAre(
                IsObjectCreationEvent(),
                IsMethodCallEvent("run"),
                IsBeginTransactionEvent())))),
            _, _, _));

    EXPECT_CALL(
        transaction_store_core,
        CreateTransaction(UnorderedElementsAre(
            Pair(_, Pointee(ElementsAre(
                IsSubMethodCallEvent("test-method")))),
            Pair(_, Pointee(ElementsAre(
                IsObjectCreationEvent(),
                IsMethodCallEvent("test-method"),
                IsEndTransactionEvent())))),
            _, _, _));

    EXPECT_CALL(
        transaction_store_core,
        CreateTransaction(UnorderedElementsAre(
            Pair(_, Pointee(ElementsAre(
                IsMethodReturnEvent()))),
            Pair(_, Pointee(ElementsAre(
                IsSubMethodReturnEvent()))),
            Pair(_, Pointee(ElementsAre(
                IsObjectCreationEvent())))),
            _, _, _));

    EXPECT_CALL(
        transaction_store_core,
        CreateTransaction(UnorderedElementsAre(
            Pair(_, Pointee(ElementsAre(
                IsMethodReturnEvent())))),
            _, _, _));
  }

  RecordingThread recording_thread(&transaction_store);
  LocalObject* const program_object =
      new CallEndTransactionFromWithinMethod_ProgramObject();

  // Run the program, which begins an explicit transaction, creates a fake local
  // object, and calls the "test-method" method on the object. That method calls
  // MethodContext::EndTransaction, creates a new object, and returns the new
  // object reference. The RecordingThread instance should create four
  // transactions:
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

  void InvokeMethod(MethodContext* method_context,
                    ObjectReference* self_object_reference,
                    const string& method_name,
                    const vector<Value>& parameters,
                    Value* return_value) override {
    CHECK_EQ(method_name, "run");

    // Create an object, and then call a method on that object in a different
    // transaction. The object should still be available in the later
    // transaction, even though the content of the object was never committed.
    // (An object is not committed until it's involved in a method call.)

    CHECK(method_context->BeginTransaction());
    ObjectReference* const object_reference1 = method_context->CreateObject(
        new FakeLocalObject("lucy."), "");
    ObjectReference* const object_reference2 = method_context->CreateObject(
        new FakeLocalObject("ethel."), "");
    // This method call is here only to force a transaction to be created.
    CallAppendMethod(method_context, object_reference1, "ricky.");
    CHECK(method_context->EndTransaction());

    CHECK(method_context->BeginTransaction());
    // object_reference2 should still be available, even though it was created
    // in an earlier transaction.
    CallAppendMethod(method_context, object_reference2, "fred.");
    CHECK(method_context->EndTransaction());
  }
};

TEST(RecordingThreadTest, CreateObjectInDifferentTransaction) {
  CanonicalPeer fake_local_peer("test-local-peer");
  MockTransactionStoreCore transaction_store_core;
  MockTransactionStore transaction_store(&transaction_store_core);

  EXPECT_CALL(transaction_store_core, GetLocalPeer())
      .WillRepeatedly(Return(&fake_local_peer));
  // TransactionStoreInternalInterface::GetLiveObjectAtSequencePoint should not
  // be called, because the thread already has a copy of the object (the only
  // copy, in fact, since the object hasn't been committed).
  EXPECT_CALL(transaction_store_core, GetLiveObjectAtSequencePoint(_, _, _))
      .Times(0);
  EXPECT_CALL(transaction_store_core, GetExecutionPhase(_))
      .WillRepeatedly(Return(TransactionStoreInternalInterface::NORMAL));

  EXPECT_CALL(transaction_store_core, CreateTransaction(_, _, _, _))
      .Times(AtLeast(2));

  RecordingThread recording_thread(&transaction_store);
  LocalObject* const program_object =
      new CreateObjectInDifferentTransaction_ProgramObject();

  Value return_value;
  recording_thread.RunProgram(program_object, "run", &return_value, false);
}

//------------------------------------------------------------------------------

class RewindInPendingTransaction_FakeLocalObject : public TestLocalObject {
 public:
  LocalObject* Clone() const override {
    return new RewindInPendingTransaction_FakeLocalObject();
  }

  void InvokeMethod(MethodContext* method_context,
                    ObjectReference* self_object_reference,
                    const string& method_name,
                    const vector<Value>& parameters,
                    Value* return_value) override {
    if (method_name == "a") {
      if (!method_context->BeginTransaction()) {
        return;
      }

      Value sub_method_return_value;
      if (!method_context->CallMethod(self_object_reference, "b",
                                      vector<Value>(),
                                      &sub_method_return_value)) {
        return;
      }

      if (!method_context->EndTransaction()) {
        return;
      }
    } else if (method_name == "b") {
      // Do nothing.
    } else {
      LOG(FATAL) << "Invalid method name: \"" << CEscape(method_name) << "\"";
    }
  }
};

class RewindInPendingTransaction_ProgramObject : public TestLocalObject {
 public:
  LocalObject* Clone() const override {
    return new RewindInPendingTransaction_ProgramObject();
  }

  void InvokeMethod(MethodContext* method_context,
                    ObjectReference* self_object_reference,
                    const string& method_name,
                    const vector<Value>& parameters,
                    Value* return_value) override {
    CHECK_EQ(method_name, "run");

    ObjectReference* const fake_local_object_reference =
        method_context->CreateObject(
            new RewindInPendingTransaction_FakeLocalObject(), "");

    Value method_return_value;
    CHECK(method_context->CallMethod(fake_local_object_reference, "a",
                                     vector<Value>(), &method_return_value));
  }
};

// TODO(dss): Enable this test once execution rewinding is working correctly.
TEST(RecordingThreadTest, DISABLED_RewindInPendingTransaction) {
  CanonicalPeer fake_local_peer("test-local-peer");
  MockTransactionStoreCore transaction_store_core;
  MockTransactionStore transaction_store(&transaction_store_core);

  EXPECT_CALL(transaction_store_core, GetLocalPeer())
      .WillRepeatedly(Return(&fake_local_peer));

  // Expected sequence:
  //
  //   Transaction 1:
  //     Method Call "run"
  //     Method Call "a"
  //
  //   Transaction 2:
  //     Begin Transaction
  //
  //   Aborted Transaction:
  //     Method Call "b"
  //
  //   <Rewind Execution; Resume Execution>
  //
  //   Replay Transaction:
  //     Begin Transaction
  //
  //   Transaction 3:
  //     Method Call "b"
  //     Method Return
  //     End Transaction
  //
  //   Transaction 4:
  //     Method Return
  //     Method Return

  Sequence s1, s2;

  EXPECT_CALL(transaction_store_core, GetExecutionPhase(_))
      .InSequence(s1)
      .WillRepeatedly(Return(TransactionStoreInternalInterface::NORMAL));

  EXPECT_CALL(
      transaction_store_core,
      CreateTransaction(UnorderedElementsAre(
          Pair(_, Pointee(ElementsAre(
              IsObjectCreationEvent(),
              IsMethodCallEvent("run"),
              IsSubMethodCallEvent("a")))),
          Pair(_, Pointee(ElementsAre(
              IsObjectCreationEvent(),
              IsMethodCallEvent("a"))))),
          _, _, _))
      .InSequence(s2);

  EXPECT_CALL(
      transaction_store_core,
      CreateTransaction(UnorderedElementsAre(
          Pair(_, Pointee(ElementsAre(
              IsBeginTransactionEvent())))),
          _, _, _))
      .InSequence(s2);

  EXPECT_CALL(transaction_store_core, GetExecutionPhase(_))
      .InSequence(s1, s2)
      .WillOnce(Return(TransactionStoreInternalInterface::REWIND))
      .WillOnce(Return(TransactionStoreInternalInterface::RESUME));

  EXPECT_CALL(transaction_store_core, GetExecutionPhase(_))
      .InSequence(s1)
      .WillRepeatedly(Return(TransactionStoreInternalInterface::NORMAL));

  EXPECT_CALL(
      transaction_store_core,
      CreateTransaction(UnorderedElementsAre(
          Pair(_, Pointee(ElementsAre(
              IsSelfMethodCallEvent("b"),
              IsSelfMethodReturnEvent(),
              IsEndTransactionEvent())))),
          _, _, _))
      .InSequence(s2);

  EXPECT_CALL(
      transaction_store_core,
      CreateTransaction(UnorderedElementsAre(
          Pair(_, Pointee(ElementsAre(
              IsMethodReturnEvent()))),
          Pair(_, Pointee(ElementsAre(
              IsSubMethodReturnEvent())))),
          _, _, _))
      .InSequence(s2);

  EXPECT_CALL(
      transaction_store_core,
      CreateTransaction(UnorderedElementsAre(
          Pair(_, Pointee(ElementsAre(
              IsMethodReturnEvent())))),
          _, _, _))
      .InSequence(s2);

  RecordingThread recording_thread(&transaction_store);
  LocalObject* const program_object =
      new RewindInPendingTransaction_ProgramObject();

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
