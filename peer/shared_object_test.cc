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

#include "peer/shared_object.h"

#include <cstddef>
#include <string>
#include <tr1/unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <gflags/gflags.h>

#include "base/linked_ptr.h"
#include "base/logging.h"
#include "fake_interpreter/fake_interpreter.h"
#include "fake_interpreter/fake_local_object.h"
#include "include/c++/interpreter.h"
#include "peer/canonical_peer.h"
#include "peer/committed_event.h"
#include "peer/committed_value.h"
#include "peer/const_live_object_ptr.h"
#include "peer/live_object.h"
#include "peer/make_transaction_id.h"
#include "peer/max_version_map.h"
#include "peer/mock_transaction_store.h"
#include "peer/proto/peer.pb.h"
#include "peer/proto/transaction_id.pb.h"
#include "peer/proto/uuid.pb.h"
#include "peer/sequence_point_impl.h"
#include "third_party/gmock-1.7.0/gtest/include/gtest/gtest.h"
#include "third_party/gmock-1.7.0/include/gmock/gmock.h"

using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
using std::pair;
using std::string;
using std::tr1::unordered_map;
using std::unordered_set;
using std::vector;
using testing::AnyNumber;
using testing::InitGoogleMock;
using testing::Test;
using testing::_;

namespace floating_temple {
namespace peer {

class PeerObjectImpl;
class SharedObject;

namespace {

ConstLiveObjectPtr MakeLocalObject(const string& s) {
  return ConstLiveObjectPtr(new LiveObject(new FakeLocalObject(s)));
}

class SharedObjectTest : public Test {
 protected:
  virtual void SetUp() {
    transaction_store_core_ = new MockTransactionStoreCore();
    transaction_store_ = new MockTransactionStore(transaction_store_core_);

    Uuid object_id;
    object_id.set_high_word(0x0123456789abcdef);
    object_id.set_low_word(0xfedcba9876543210);

    shared_object_ = new SharedObject(transaction_store_, object_id);
  }

  virtual void TearDown() {
    delete shared_object_;
    delete transaction_store_;
    delete transaction_store_core_;
  }

  void InsertObjectCreationTransaction(const CanonicalPeer* origin_peer,
                                       const TransactionId& transaction_id,
                                       const string& initial_string) {
    vector<linked_ptr<CommittedEvent> > events;

    AddEventToVector(
        new ObjectCreationCommittedEvent(MakeLocalObject(initial_string)),
        &events);

    shared_object_->InsertTransaction(origin_peer, transaction_id, &events);
  }

  void InsertAppendTransaction(const CanonicalPeer* origin_peer,
                               const TransactionId& transaction_id,
                               const string& string_to_append) {
    vector<linked_ptr<CommittedEvent> > events;

    vector<CommittedValue> parameters(1);
    parameters[0].set_local_type(FakeLocalObject::kStringLocalType);
    parameters[0].set_string_value(string_to_append);

    CommittedValue return_value;
    return_value.set_local_type(FakeLocalObject::kVoidLocalType);
    return_value.set_empty();

    const unordered_set<SharedObject*> new_shared_objects;

    AddEventToVector(
        new MethodCallCommittedEvent(NULL, "append", parameters),
        &events);
    AddEventToVector(
        new MethodReturnCommittedEvent(new_shared_objects, NULL, return_value),
        &events);

    shared_object_->InsertTransaction(origin_peer, transaction_id, &events);
  }

  void InsertAppendGetTransaction(const CanonicalPeer* origin_peer,
                                  const TransactionId& transaction_id,
                                  const string& string_to_append,
                                  const string& expected_result_string) {
    vector<linked_ptr<CommittedEvent> > events;
    const unordered_set<SharedObject*> new_shared_objects;

    {
      vector<CommittedValue> parameters(1);
      parameters[0].set_local_type(FakeLocalObject::kStringLocalType);
      parameters[0].set_string_value(string_to_append);

      CommittedValue return_value;
      return_value.set_local_type(FakeLocalObject::kVoidLocalType);
      return_value.set_empty();

      AddEventToVector(
          new MethodCallCommittedEvent(NULL, "append", parameters),
          &events);
      AddEventToVector(
          new MethodReturnCommittedEvent(new_shared_objects, NULL,
                                         return_value),
          &events);
    }

    {
      CommittedValue return_value;
      return_value.set_local_type(FakeLocalObject::kStringLocalType);
      return_value.set_string_value(expected_result_string);

      AddEventToVector(
          new MethodCallCommittedEvent(NULL, "get", vector<CommittedValue>()),
          &events);
      AddEventToVector(
          new MethodReturnCommittedEvent(new_shared_objects, NULL,
                                         return_value),
          &events);
    }

    shared_object_->InsertTransaction(origin_peer, transaction_id, &events);
  }

  void AddEventToVector(CommittedEvent* event,
                        vector<linked_ptr<CommittedEvent> >* events) {
    CHECK(event != NULL);
    CHECK(events != NULL);

    events->push_back(make_linked_ptr(event));
  }

  MockTransactionStoreCore* transaction_store_core_;
  MockTransactionStore* transaction_store_;
  SharedObject* shared_object_;
};

TEST_F(SharedObjectTest, InsertObjectCreationAfterTransaction) {
  EXPECT_CALL(*transaction_store_core_, GetCurrentSequencePoint())
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, GetLiveObjectAtSequencePoint(_, _, _))
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, CreateUnboundPeerObject())
      .Times(AnyNumber());
  EXPECT_CALL(*transaction_store_core_, GetOrCreateNamedObject(_))
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, CreateTransaction(_, _, _, _))
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, ObjectsAreEquivalent(_, _))
      .Times(0);

  const CanonicalPeer canonical_peer1("peer_a");
  const CanonicalPeer canonical_peer2("peer_b");

  InsertAppendTransaction(&canonical_peer2, MakeTransactionId(20, 0, 0),
                          "banana.");

  InsertObjectCreationTransaction(&canonical_peer1, MakeTransactionId(10, 0, 0),
                                  "apple.");

  // No working version should be available at version { "peer_b": 20 }. The
  // OBJECT_CREATION event has version map { "peer_a": 10 }, but the requested
  // version has no entry for "peer_a".
  {
    SequencePointImpl sequence_point;
    sequence_point.AddPeerTransactionId(&canonical_peer2,
                                        MakeTransactionId(20, 0, 0));

    unordered_map<SharedObject*, PeerObjectImpl*> new_peer_objects;
    vector<pair<const CanonicalPeer*, TransactionId> > transactions_to_reject;

    EXPECT_TRUE(shared_object_->GetWorkingVersion(
                    MaxVersionMap(), sequence_point, &new_peer_objects,
                    &transactions_to_reject).get() == NULL);

    EXPECT_EQ(0u, new_peer_objects.size());
    EXPECT_EQ(0u, transactions_to_reject.size());
  }

  {
    SequencePointImpl sequence_point;
    sequence_point.AddPeerTransactionId(&canonical_peer1,
                                        MakeTransactionId(10, 0, 0));
    sequence_point.AddPeerTransactionId(&canonical_peer2,
                                        MakeTransactionId(20, 0, 0));

    unordered_map<SharedObject*, PeerObjectImpl*> new_peer_objects;
    vector<pair<const CanonicalPeer*, TransactionId> > transactions_to_reject;

    EXPECT_EQ("apple.banana.",
              static_cast<const FakeLocalObject*>(
                  shared_object_->GetWorkingVersion(MaxVersionMap(),
                                                    sequence_point,
                                                    &new_peer_objects,
                                                    &transactions_to_reject)->
                  local_object())->s());

    EXPECT_EQ(0u, new_peer_objects.size());
    EXPECT_EQ(0u, transactions_to_reject.size());
  }
}

TEST_F(SharedObjectTest, InsertObjectCreationWithConflict) {
  EXPECT_CALL(*transaction_store_core_, GetCurrentSequencePoint())
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, GetLiveObjectAtSequencePoint(_, _, _))
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, CreateUnboundPeerObject())
      .Times(AnyNumber());
  EXPECT_CALL(*transaction_store_core_, GetOrCreateNamedObject(_))
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, CreateTransaction(_, _, _, _))
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, ObjectsAreEquivalent(_, _))
      .Times(0);

  const CanonicalPeer canonical_peer1("peer_a");
  const CanonicalPeer canonical_peer2("peer_b");

  // Intentionally specify the wrong return value for the "get" method so that
  // this transaction will be deleted. (When invoked, the actual "get" method
  // will return "apple.banana.", not "apple.durian.".)
  InsertAppendGetTransaction(&canonical_peer2, MakeTransactionId(20, 0, 0),
                             "banana.", "apple.durian.");

  InsertAppendTransaction(&canonical_peer1, MakeTransactionId(30, 0, 0),
                          "cherry.");

  InsertObjectCreationTransaction(&canonical_peer1, MakeTransactionId(10, 0, 0),
                                  "apple.");

  {
    SequencePointImpl sequence_point;
    sequence_point.AddPeerTransactionId(&canonical_peer1,
                                        MakeTransactionId(10, 0, 0));

    unordered_map<SharedObject*, PeerObjectImpl*> new_peer_objects;
    vector<pair<const CanonicalPeer*, TransactionId> > transactions_to_reject;

    EXPECT_EQ("apple.",
              static_cast<const FakeLocalObject*>(
                  shared_object_->GetWorkingVersion(MaxVersionMap(),
                                                    sequence_point,
                                                    &new_peer_objects,
                                                    &transactions_to_reject)->
                  local_object())->s());

    EXPECT_EQ(0u, new_peer_objects.size());
    EXPECT_EQ(0u, transactions_to_reject.size());
  }

  {
    SequencePointImpl sequence_point;
    sequence_point.AddPeerTransactionId(&canonical_peer1,
                                        MakeTransactionId(10, 0, 0));
    sequence_point.AddPeerTransactionId(&canonical_peer2,
                                        MakeTransactionId(20, 0, 0));

    unordered_map<SharedObject*, PeerObjectImpl*> new_peer_objects;
    vector<pair<const CanonicalPeer*, TransactionId> > transactions_to_reject;

    EXPECT_EQ("apple.",
              static_cast<const FakeLocalObject*>(
                  shared_object_->GetWorkingVersion(MaxVersionMap(),
                                                    sequence_point,
                                                    &new_peer_objects,
                                                    &transactions_to_reject)->
                  local_object())->s());

    EXPECT_EQ(0u, new_peer_objects.size());
    ASSERT_EQ(1u, transactions_to_reject.size());

    EXPECT_EQ(&canonical_peer2, transactions_to_reject[0].first);
    EXPECT_EQ(20u, transactions_to_reject[0].second.a());
  }

  {
    SequencePointImpl sequence_point;
    sequence_point.AddPeerTransactionId(&canonical_peer1,
                                        MakeTransactionId(30, 0, 0));
    sequence_point.AddPeerTransactionId(&canonical_peer2,
                                        MakeTransactionId(20, 0, 0));

    unordered_map<SharedObject*, PeerObjectImpl*> new_peer_objects;
    vector<pair<const CanonicalPeer*, TransactionId> > transactions_to_reject;

    EXPECT_EQ("apple.cherry.",
              static_cast<const FakeLocalObject*>(
                  shared_object_->GetWorkingVersion(MaxVersionMap(),
                                                    sequence_point,
                                                    &new_peer_objects,
                                                    &transactions_to_reject)->
                  local_object())->s());

    EXPECT_EQ(0u, new_peer_objects.size());
    ASSERT_EQ(1u, transactions_to_reject.size());

    EXPECT_EQ(&canonical_peer2, transactions_to_reject[0].first);
    EXPECT_EQ(20u, transactions_to_reject[0].second.a());
  }
}

TEST_F(SharedObjectTest, GetWorkingVersionWithConflict) {
  EXPECT_CALL(*transaction_store_core_, GetCurrentSequencePoint())
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, GetLiveObjectAtSequencePoint(_, _, _))
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, CreateUnboundPeerObject())
      .Times(AnyNumber());
  EXPECT_CALL(*transaction_store_core_, GetOrCreateNamedObject(_))
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, CreateTransaction(_, _, _, _))
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, ObjectsAreEquivalent(_, _))
      .Times(0);

  const CanonicalPeer canonical_peer1("peer_a");
  const CanonicalPeer canonical_peer2("peer_b");
  const CanonicalPeer canonical_peer3("peer_c");

  InsertAppendGetTransaction(&canonical_peer3, MakeTransactionId(30, 0, 0),
                             "cherry.", "apple.banana.cherry.");

  InsertAppendTransaction(&canonical_peer2, MakeTransactionId(20, 0, 0),
                          "banana.");

  InsertObjectCreationTransaction(&canonical_peer1, MakeTransactionId(10, 0, 0),
                                  "apple.");

  {
    SequencePointImpl sequence_point;
    sequence_point.AddPeerTransactionId(&canonical_peer1,
                                        MakeTransactionId(10, 0, 0));

    unordered_map<SharedObject*, PeerObjectImpl*> new_peer_objects;
    vector<pair<const CanonicalPeer*, TransactionId> > transactions_to_reject;

    EXPECT_EQ("apple.",
              static_cast<const FakeLocalObject*>(
                  shared_object_->GetWorkingVersion(MaxVersionMap(),
                                                    sequence_point,
                                                    &new_peer_objects,
                                                    &transactions_to_reject)->
                  local_object())->s());

    EXPECT_EQ(0u, new_peer_objects.size());
    EXPECT_EQ(0u, transactions_to_reject.size());
  }

  {
    SequencePointImpl sequence_point;
    sequence_point.AddPeerTransactionId(&canonical_peer1,
                                        MakeTransactionId(10, 0, 0));
    sequence_point.AddPeerTransactionId(&canonical_peer3,
                                        MakeTransactionId(30, 0, 0));

    unordered_map<SharedObject*, PeerObjectImpl*> new_peer_objects;
    vector<pair<const CanonicalPeer*, TransactionId> > transactions_to_reject;

    EXPECT_EQ("apple.",
              static_cast<const FakeLocalObject*>(
                  shared_object_->GetWorkingVersion(MaxVersionMap(),
                                                    sequence_point,
                                                    &new_peer_objects,
                                                    &transactions_to_reject)->
                  local_object())->s());

    EXPECT_EQ(0u, new_peer_objects.size());
    ASSERT_EQ(1u, transactions_to_reject.size());

    EXPECT_EQ(&canonical_peer3, transactions_to_reject[0].first);
    EXPECT_EQ(30u, transactions_to_reject[0].second.a());
  }

  {
    SequencePointImpl sequence_point;
    sequence_point.AddPeerTransactionId(&canonical_peer1,
                                        MakeTransactionId(10, 0, 0));
    sequence_point.AddPeerTransactionId(&canonical_peer2,
                                        MakeTransactionId(20, 0, 0));
    sequence_point.AddPeerTransactionId(&canonical_peer3,
                                        MakeTransactionId(30, 0, 0));

    unordered_map<SharedObject*, PeerObjectImpl*> new_peer_objects;
    vector<pair<const CanonicalPeer*, TransactionId> > transactions_to_reject;

    EXPECT_EQ("apple.banana.cherry.",
              static_cast<const FakeLocalObject*>(
                  shared_object_->GetWorkingVersion(MaxVersionMap(),
                                                    sequence_point,
                                                    &new_peer_objects,
                                                    &transactions_to_reject)->
                  local_object())->s());

    EXPECT_EQ(0u, new_peer_objects.size());
    EXPECT_EQ(0u, transactions_to_reject.size());
  }
}

TEST_F(SharedObjectTest, InsertTransactionWithInitialVersion) {
  EXPECT_CALL(*transaction_store_core_, GetCurrentSequencePoint())
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, GetLiveObjectAtSequencePoint(_, _, _))
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, CreateUnboundPeerObject())
      .Times(AnyNumber());
  EXPECT_CALL(*transaction_store_core_, GetOrCreateNamedObject(_))
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, CreateTransaction(_, _, _, _))
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, ObjectsAreEquivalent(_, _))
      .Times(0);

  const CanonicalPeer canonical_peer("peer_a");

  {
    vector<linked_ptr<CommittedEvent> > events;

    vector<CommittedValue> parameters(1);
    parameters[0].set_local_type(FakeLocalObject::kStringLocalType);
    parameters[0].set_string_value("whatcha playin'?");

    CommittedValue return_value;
    return_value.set_local_type(FakeLocalObject::kVoidLocalType);
    return_value.set_empty();

    const unordered_set<SharedObject*> new_shared_objects;

    AddEventToVector(
        new MethodCallCommittedEvent(NULL, "append", parameters),
        &events);
    AddEventToVector(
        new MethodReturnCommittedEvent(new_shared_objects, NULL, return_value),
        &events);

    shared_object_->InsertTransaction(&canonical_peer,
                                      MakeTransactionId(100, 0, 0), &events);
  }

  InsertObjectCreationTransaction(&canonical_peer, MakeTransactionId(50, 0, 0),
                                  "Hey Ash, ");

  {
    SequencePointImpl sequence_point;
    sequence_point.AddPeerTransactionId(&canonical_peer,
                                        MakeTransactionId(100, 0, 0));

    unordered_map<SharedObject*, PeerObjectImpl*> new_peer_objects;
    vector<pair<const CanonicalPeer*, TransactionId> > transactions_to_reject;

    const ConstLiveObjectPtr live_object = shared_object_->GetWorkingVersion(
        MaxVersionMap(), sequence_point, &new_peer_objects,
        &transactions_to_reject);

    EXPECT_EQ(0u, new_peer_objects.size());
    EXPECT_EQ(0u, transactions_to_reject.size());

    EXPECT_EQ("Hey Ash, whatcha playin'?",
              static_cast<const FakeLocalObject*>(
                  live_object->local_object())->s());
  }
}

TEST_F(SharedObjectTest, MethodCallAndMethodReturnAsSeparateTransactions) {
  EXPECT_CALL(*transaction_store_core_, GetCurrentSequencePoint())
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, GetLiveObjectAtSequencePoint(_, _, _))
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, CreateUnboundPeerObject())
      .Times(AnyNumber());
  EXPECT_CALL(*transaction_store_core_, GetOrCreateNamedObject(_))
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, CreateTransaction(_, _, _, _))
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, ObjectsAreEquivalent(_, _))
      .Times(0);

  const CanonicalPeer canonical_peer("peer_a");

  {
    vector<linked_ptr<CommittedEvent> > events;

    ConstLiveObjectPtr initial_live_object = MakeLocalObject("I don't know. ");

    vector<CommittedValue> parameters(1);
    parameters[0].set_local_type(FakeLocalObject::kStringLocalType);
    parameters[0].set_string_value("Third base.");

    AddEventToVector(
        new ObjectCreationCommittedEvent(initial_live_object),
        &events);
    AddEventToVector(
        new MethodCallCommittedEvent(NULL, "append", parameters),
        &events);

    shared_object_->InsertTransaction(&canonical_peer,
                                      MakeTransactionId(100, 0, 0), &events);
  }

  {
    vector<linked_ptr<CommittedEvent> > events;

    CommittedValue return_value;
    return_value.set_local_type(FakeLocalObject::kVoidLocalType);
    return_value.set_empty();

    const unordered_set<SharedObject*> new_shared_objects;

    AddEventToVector(
        new MethodReturnCommittedEvent(new_shared_objects, NULL, return_value),
        &events);

    shared_object_->InsertTransaction(&canonical_peer,
                                      MakeTransactionId(200, 0, 0), &events);
  }

  {
    SequencePointImpl sequence_point;
    sequence_point.AddPeerTransactionId(&canonical_peer,
                                        MakeTransactionId(200, 0, 0));

    unordered_map<SharedObject*, PeerObjectImpl*> new_peer_objects;
    vector<pair<const CanonicalPeer*, TransactionId> > transactions_to_reject;

    const ConstLiveObjectPtr live_object = shared_object_->GetWorkingVersion(
        MaxVersionMap(), sequence_point, &new_peer_objects,
        &transactions_to_reject);

    EXPECT_EQ(0u, new_peer_objects.size());
    EXPECT_EQ(0u, transactions_to_reject.size());

    EXPECT_EQ("I don't know. Third base.",
              static_cast<const FakeLocalObject*>(
                  live_object->local_object())->s());
  }
}

TEST_F(SharedObjectTest, BackingUp) {
  EXPECT_CALL(*transaction_store_core_, GetCurrentSequencePoint())
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, GetLiveObjectAtSequencePoint(_, _, _))
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, CreateUnboundPeerObject())
      .Times(AnyNumber());
  EXPECT_CALL(*transaction_store_core_, GetOrCreateNamedObject(_))
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, CreateTransaction(_, _, _, _))
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, ObjectsAreEquivalent(_, _))
      .Times(0);

  const CanonicalPeer canonical_peer("peer_a");

  // Insert three consecutive transactions. When replaying the transactions, the
  // shared object will have to back up to the first transaction, because the
  // second and third transactions do not begin with METHOD_CALL events.

  {
    vector<linked_ptr<CommittedEvent> > events;

    ConstLiveObjectPtr initial_live_object = MakeLocalObject("Game. ");

    vector<CommittedValue> parameters(1);
    parameters[0].set_local_type(FakeLocalObject::kStringLocalType);
    parameters[0].set_string_value("Set. ");

    AddEventToVector(
        new ObjectCreationCommittedEvent(initial_live_object),
        &events);
    AddEventToVector(
        new MethodCallCommittedEvent(NULL, "append", parameters),
        &events);

    shared_object_->InsertTransaction(&canonical_peer,
                                      MakeTransactionId(100, 0, 0), &events);
  }

  {
    vector<linked_ptr<CommittedEvent> > events;

    CommittedValue return_value;
    return_value.set_local_type(FakeLocalObject::kVoidLocalType);
    return_value.set_empty();

    const unordered_set<SharedObject*> new_shared_objects;

    vector<CommittedValue> parameters(1);
    parameters[0].set_local_type(FakeLocalObject::kStringLocalType);
    parameters[0].set_string_value("Match.");

    AddEventToVector(
        new MethodReturnCommittedEvent(new_shared_objects, NULL, return_value),
        &events);
    AddEventToVector(
        new MethodCallCommittedEvent(NULL, "append", parameters),
        &events);

    shared_object_->InsertTransaction(&canonical_peer,
                                      MakeTransactionId(200, 0, 0), &events);
  }

  {
    vector<linked_ptr<CommittedEvent> > events;

    CommittedValue return_value;
    return_value.set_local_type(FakeLocalObject::kVoidLocalType);
    return_value.set_empty();

    const unordered_set<SharedObject*> new_shared_objects;

    AddEventToVector(
        new MethodReturnCommittedEvent(new_shared_objects, NULL, return_value),
        &events);

    shared_object_->InsertTransaction(&canonical_peer,
                                      MakeTransactionId(300, 0, 0), &events);
  }

  {
    SequencePointImpl sequence_point;
    sequence_point.AddPeerTransactionId(&canonical_peer,
                                        MakeTransactionId(300, 0, 0));

    unordered_map<SharedObject*, PeerObjectImpl*> new_peer_objects;
    vector<pair<const CanonicalPeer*, TransactionId> > transactions_to_reject;

    const ConstLiveObjectPtr live_object = shared_object_->GetWorkingVersion(
        MaxVersionMap(), sequence_point, &new_peer_objects,
        &transactions_to_reject);

    EXPECT_EQ(0u, new_peer_objects.size());
    EXPECT_EQ(0u, transactions_to_reject.size());

    EXPECT_EQ("Game. Set. Match.",
              static_cast<const FakeLocalObject*>(
                  live_object->local_object())->s());
  }
}

TEST_F(SharedObjectTest, MultipleObjectCreationEvents) {
  EXPECT_CALL(*transaction_store_core_, GetCurrentSequencePoint())
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, GetLiveObjectAtSequencePoint(_, _, _))
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, CreateUnboundPeerObject())
      .Times(AnyNumber());
  EXPECT_CALL(*transaction_store_core_, GetOrCreateNamedObject(_))
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, CreateTransaction(_, _, _, _))
      .Times(0);
  EXPECT_CALL(*transaction_store_core_, ObjectsAreEquivalent(_, _))
      .Times(0);

  const CanonicalPeer canonical_peer1("peer_a");
  const CanonicalPeer canonical_peer2("peer_b");

  // Transaction #1: OBJECT_CREATION
  InsertObjectCreationTransaction(&canonical_peer1, MakeTransactionId(10, 0, 0),
                                  "joker.");

  // Transaction #2: METHOD_CALL + METHOD_RETURN
  InsertAppendTransaction(&canonical_peer1, MakeTransactionId(20, 0, 0),
                          "penguin.");

  // Transaction #3: OBJECT_CREATION
  InsertObjectCreationTransaction(&canonical_peer2, MakeTransactionId(30, 0, 0),
                                  "batman.");

  // Call SharedObject::GetWorkingVersion and pass in a sequence point that only
  // includes Transaction #3. The method should return the local object that's
  // contained in the second OBJECT_CREATION event.
  //
  // This simulates the scenario where the local peer has received the contents
  // of the shared object from a remote peer, but the currently executing local
  // transaction is still using an outdated version of the object.
  {
    SequencePointImpl sequence_point;
    sequence_point.AddPeerTransactionId(&canonical_peer2,
                                        MakeTransactionId(30, 0, 0));

    unordered_map<SharedObject*, PeerObjectImpl*> new_peer_objects;
    vector<pair<const CanonicalPeer*, TransactionId> > transactions_to_reject;

    EXPECT_EQ("batman.",
              static_cast<const FakeLocalObject*>(
                  shared_object_->GetWorkingVersion(MaxVersionMap(),
                                                    sequence_point,
                                                    &new_peer_objects,
                                                    &transactions_to_reject)->
                  local_object())->s());

    EXPECT_EQ(0u, new_peer_objects.size());
    EXPECT_EQ(0u, transactions_to_reject.size());
  }
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
