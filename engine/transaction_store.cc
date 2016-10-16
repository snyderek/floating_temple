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
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/cond_var.h"
#include "base/integral_types.h"
#include "base/logging.h"
#include "base/mutex.h"
#include "base/mutex_lock.h"
#include "engine/canonical_peer.h"
#include "engine/canonical_peer_map.h"
#include "engine/committed_event.h"
#include "engine/convert_value.h"
#include "engine/get_event_proto_type.h"
#include "engine/get_peer_message_type.h"
#include "engine/live_object.h"
#include "engine/max_version_map.h"
#include "engine/min_version_map.h"
#include "engine/object_reference_impl.h"
#include "engine/peer_message_sender.h"
#include "engine/proto/event.pb.h"
#include "engine/proto/peer.pb.h"
#include "engine/proto/transaction_id.pb.h"
#include "engine/proto/uuid.pb.h"
#include "engine/proto/value_proto.pb.h"
#include "engine/recording_thread.h"
#include "engine/sequence_point_impl.h"
#include "engine/serialize_local_object_to_string.h"
#include "engine/shared_object.h"
#include "engine/shared_object_transaction.h"
#include "engine/transaction_id_generator.h"
#include "engine/transaction_id_util.h"
#include "engine/transaction_sequencer.h"
#include "engine/uuid_util.h"
#include "engine/value_proto_util.h"
#include "include/c++/value.h"
#include "util/dump_context_impl.h"

using std::map;
using std::pair;
using std::shared_ptr;
using std::size_t;
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::unordered_set;
using std::vector;

namespace floating_temple {
namespace engine {

const char TransactionStore::kObjectNamespaceUuidString[] =
    "ab2d0b40fe6211e2bf8b000c2949fc67";

TransactionStore::TransactionStore(CanonicalPeerMap* canonical_peer_map,
                                   PeerMessageSender* peer_message_sender,
                                   Interpreter* interpreter,
                                   const CanonicalPeer* local_peer)
    : canonical_peer_map_(CHECK_NOTNULL(canonical_peer_map)),
      interpreter_(CHECK_NOTNULL(interpreter)),
      local_peer_(CHECK_NOTNULL(local_peer)),
      object_namespace_uuid_(StringToUuid(kObjectNamespaceUuidString)),
      transaction_sequencer_(canonical_peer_map, peer_message_sender,
                             &transaction_id_generator_, local_peer),
      recording_thread_(nullptr),
      rejected_transaction_id_(MIN_TRANSACTION_ID),
      version_number_(1) {
  TransactionId initial_transaction_id;
  transaction_id_generator_.Generate(&initial_transaction_id);

  current_sequence_point_.AddPeerTransactionId(local_peer,
                                               initial_transaction_id);
}

TransactionStore::~TransactionStore() {
}

void TransactionStore::RunProgram(LocalObject* local_object,
                                  const string& method_name,
                                  Value* return_value,
                                  bool linger) {
  RecordingThread thread(this);

  {
    MutexLock lock(&recording_thread_mu_);
    CHECK(recording_thread_ == nullptr);
    recording_thread_ = &thread;
  }

  thread.RunProgram(local_object, method_name, return_value, linger);

  {
    MutexLock lock(&recording_thread_mu_);
    CHECK(recording_thread_ == &thread);
    recording_thread_ = nullptr;
  }
}

void TransactionStore::NotifyNewConnection(const CanonicalPeer* remote_peer) {
  unordered_set<SharedObject*> named_objects;
  {
    MutexLock lock(&named_objects_mu_);
    named_objects = named_objects_;
  }

  for (const SharedObject* const shared_object : named_objects) {
    PeerMessage peer_message;
    GetObjectMessage* const get_object_message =
        peer_message.mutable_get_object_message();
    get_object_message->mutable_object_id()->CopyFrom(
        shared_object->object_id());

    transaction_sequencer_.SendMessageToRemotePeer(
        remote_peer, peer_message, PeerMessageSender::NON_BLOCKING_MODE);
  }
}

#define HANDLE_PEER_MESSAGE(enum_const, method_name, field_name) \
  case PeerMessage::enum_const: \
    method_name(remote_peer, peer_message.field_name()); \
    break;

void TransactionStore::HandleMessageFromRemotePeer(
    const CanonicalPeer* remote_peer, const PeerMessage& peer_message) {
  CHECK(remote_peer != nullptr);

  const PeerMessage::Type peer_message_type = GetPeerMessageType(peer_message);

  switch (peer_message_type) {
    HANDLE_PEER_MESSAGE(APPLY_TRANSACTION,
                        HandleApplyTransactionMessage,
                        apply_transaction_message);
    HANDLE_PEER_MESSAGE(GET_OBJECT,
                        HandleGetObjectMessage,
                        get_object_message);
    HANDLE_PEER_MESSAGE(STORE_OBJECT,
                        HandleStoreObjectMessage,
                        store_object_message);
    HANDLE_PEER_MESSAGE(REJECT_TRANSACTION,
                        HandleRejectTransactionMessage,
                        reject_transaction_message);
    HANDLE_PEER_MESSAGE(INVALIDATE_TRANSACTIONS,
                        HandleInvalidateTransactionsMessage,
                        invalidate_transactions_message);

    default:
      LOG(FATAL) << "Unexpected peer message type: " << peer_message_type;
  }
}

#undef HANDLE_PEER_MESSAGE

size_t TransactionStore::UuidHasher::operator()(const Uuid& uuid) const {
  return static_cast<size_t>(uuid.high_word());
}

bool TransactionStore::UuidEquals::operator()(const Uuid& a,
                                              const Uuid& b) const {
  return CompareUuids(a, b) == 0;
}

const CanonicalPeer* TransactionStore::GetLocalPeer() const {
  return local_peer_;
}

SequencePoint* TransactionStore::GetCurrentSequencePoint() const {
  MutexLock lock(&current_sequence_point_mu_);
  return current_sequence_point_.Clone();
}

shared_ptr<const LiveObject> TransactionStore::GetLiveObjectAtSequencePoint(
    ObjectReferenceImpl* object_reference, const SequencePoint* sequence_point,
    bool wait) {
  CHECK(object_reference != nullptr);

  SharedObject* const shared_object = object_reference->shared_object();
  // The object must have been created by a committed transaction, because
  // otherwise the pending transaction wouldn't need to request it. Therefore a
  // shared object should exist for the object reference.
  CHECK(shared_object != nullptr);

  const SequencePointImpl* const sequence_point_impl =
      static_cast<const SequencePointImpl*>(sequence_point);

  uint64 current_version_number = 0;
  vector<pair<const CanonicalPeer*, TransactionId>> all_transactions_to_reject;

  shared_ptr<const LiveObject> live_object =
      GetLiveObjectAtSequencePoint_Helper(shared_object, *sequence_point_impl,
                                          &current_version_number,
                                          &all_transactions_to_reject);

  if (live_object.get() == nullptr) {
    PeerMessage peer_message;
    GetObjectMessage* const get_object_message =
        peer_message.mutable_get_object_message();
    get_object_message->mutable_object_id()->CopyFrom(
        shared_object->object_id());

    transaction_sequencer_.BroadcastMessage(peer_message,
                                            PeerMessageSender::BLOCKING_MODE);

    if (wait) {
      while (live_object.get() == nullptr) {
        live_object = GetLiveObjectAtSequencePoint_Helper(
            shared_object, *sequence_point_impl, &current_version_number,
            &all_transactions_to_reject);
      }
    }
  }

  TransactionId new_transaction_id;
  transaction_sequencer_.ReserveTransaction(&new_transaction_id);

  RejectTransactionsAndSendMessages(all_transactions_to_reject,
                                    new_transaction_id);

  transaction_sequencer_.ReleaseTransaction(new_transaction_id);

  UpdateCurrentSequencePoint(local_peer_, new_transaction_id);

  return live_object;
}

ObjectReferenceImpl* TransactionStore::CreateBoundObjectReference(
    const string& name) {
  if (name.empty()) {
    Uuid object_id;
    GenerateUuid(&object_id);

    SharedObject* const shared_object = new SharedObject(this, object_id);
    // TODO(dss): [BUG] Garbage-collect ObjectReferenceImpl instances when
    // they're no longer being used.
    ObjectReferenceImpl* const object_reference = new ObjectReferenceImpl(
        shared_object);
    shared_object->AddObjectReference(object_reference);

    {
      MutexLock lock(&shared_objects_mu_);
      CHECK(shared_objects_.emplace(
                object_id, unique_ptr<SharedObject>(shared_object)).second);
    }

    return object_reference;
  } else {
    Uuid object_id;
    GeneratePredictableUuid(object_namespace_uuid_, name, &object_id);

    SharedObject* const shared_object = GetOrCreateSharedObject(object_id);

    {
      MutexLock lock(&named_objects_mu_);
      named_objects_.insert(shared_object);
    }

    return shared_object->GetOrCreateObjectReference();
  }
}

void TransactionStore::CreateTransaction(
    const unordered_map<ObjectReferenceImpl*,
                        unique_ptr<SharedObjectTransaction>>&
        object_transactions,
    TransactionId* transaction_id,
    const unordered_map<ObjectReferenceImpl*, shared_ptr<LiveObject>>&
        modified_objects,
    const SequencePoint* prev_sequence_point) {
  CHECK(transaction_id != nullptr);
  CHECK(prev_sequence_point != nullptr);

  TransactionId transaction_id_temp;
  transaction_sequencer_.ReserveTransaction(&transaction_id_temp);

  unordered_map<SharedObject*, unique_ptr<SharedObjectTransaction>>
      shared_object_transactions;
  shared_object_transactions.reserve(object_transactions.size());
  for (const auto& transaction_pair : object_transactions) {
    ObjectReferenceImpl* const object_reference = transaction_pair.first;
    const SharedObjectTransaction* const transaction =
        transaction_pair.second.get();

    SharedObject* const shared_object = GetSharedObjectForObjectReference(
        object_reference);
    EnsureSharedObjectsInTransactionExist(transaction);

    unique_ptr<SharedObjectTransaction>& transaction_clone =
        shared_object_transactions[shared_object];
    CHECK(transaction_clone.get() == nullptr);
    // TODO(dss): Cloning the SharedObjectTransaction instance here seems
    // unnecessarily inefficient.
    transaction_clone.reset(transaction->Clone());
  }

  ApplyTransactionAndSendMessage(transaction_id_temp,
                                 shared_object_transactions);

  transaction_sequencer_.ReleaseTransaction(transaction_id_temp);

  SequencePointImpl cached_version_sequence_point;
  cached_version_sequence_point.CopyFrom(
      *static_cast<const SequencePointImpl*>(prev_sequence_point));
  cached_version_sequence_point.AddPeerTransactionId(local_peer_,
                                                     transaction_id_temp);

  for (const auto& modified_object_pair : modified_objects) {
    ObjectReferenceImpl* const object_reference = modified_object_pair.first;
    const shared_ptr<const LiveObject> live_object =
        modified_object_pair.second;

    SharedObject* const shared_object = object_reference->shared_object();

    if (shared_object != nullptr) {
      shared_object->SetCachedLiveObject(live_object,
                                         cached_version_sequence_point);
    }
  }

  transaction_id->Swap(&transaction_id_temp);
}

bool TransactionStore::ObjectsAreIdentical(const ObjectReferenceImpl* a,
                                           const ObjectReferenceImpl* b) const {
  // TODO(dss): Move this code to PlaybackThread::ObjectsAreIdentical.

  CHECK(a != nullptr);
  CHECK(b != nullptr);

  if (a == b) {
    return true;
  }

  const SharedObject* const a_shared_object = a->shared_object();
  const SharedObject* const b_shared_object = b->shared_object();

  return a_shared_object != nullptr && a_shared_object == b_shared_object;
}

TransactionStoreInternalInterface::ExecutionPhase
TransactionStore::GetExecutionPhase(const TransactionId& base_transaction_id) {
  MutexLock lock(&rejected_transaction_id_mu_);

  if (rejected_transaction_id_ == MIN_TRANSACTION_ID) {
    return NORMAL;
  } else {
    if (base_transaction_id >= rejected_transaction_id_) {
      return REWIND;
    } else {
      // Clear the rewind state.
      rejected_transaction_id_ = MIN_TRANSACTION_ID;
      return RESUME;
    }
  }
}

void TransactionStore::WaitForRewind() {
  MutexLock lock(&rejected_transaction_id_mu_);

  while (rejected_transaction_id_ == MIN_TRANSACTION_ID) {
    rewinding_cond_.WaitPatiently(&rejected_transaction_id_mu_);
  }

  // Clear the rewind state.
  rejected_transaction_id_ = MIN_TRANSACTION_ID;
}

void TransactionStore::HandleApplyTransactionMessage(
    const CanonicalPeer* remote_peer,
    const ApplyTransactionMessage& apply_transaction_message) {
  CHECK(remote_peer != nullptr);

  const TransactionId& transaction_id =
      apply_transaction_message.transaction_id();

  unordered_map<SharedObject*, unique_ptr<SharedObjectTransaction>>
      shared_object_transactions;

  for (int i = 0; i < apply_transaction_message.object_transaction_size();
       ++i) {
    const ObjectTransactionProto& object_transaction =
        apply_transaction_message.object_transaction(i);

    SharedObject* const shared_object = GetSharedObject(
        object_transaction.object_id());

    if (shared_object != nullptr) {
      const int event_count = object_transaction.event_size();

      vector<unique_ptr<CommittedEvent>> events;
      events.resize(event_count);

      for (int j = 0; j < event_count; ++j) {
        const EventProto& event_proto = object_transaction.event(j);
        events[j].reset(ConvertEventProtoToCommittedEvent(event_proto));
      }

      SharedObjectTransaction* const transaction = new SharedObjectTransaction(
          events, remote_peer);
      // TODO(dss): Fail gracefully if the remote peer sent a transaction with a
      // repeated object ID.
      CHECK(shared_object_transactions.emplace(
          shared_object,
          unique_ptr<SharedObjectTransaction>(transaction)).second);
    }
  }

  ApplyTransaction(transaction_id, remote_peer, shared_object_transactions);
}

void TransactionStore::HandleGetObjectMessage(
    const CanonicalPeer* remote_peer,
    const GetObjectMessage& get_object_message) {
  CHECK(remote_peer != nullptr);

  const Uuid& requested_object_id = get_object_message.object_id();

  SharedObject* const requested_shared_object = GetSharedObject(
      requested_object_id);

  if (requested_shared_object == nullptr) {
    VLOG(1) << "The remote peer " << remote_peer->peer_id() << " requested the "
            << "object " << UuidToString(requested_object_id) << " but it does "
            << "not exist on this peer.";

    // TODO(dss): Is there any point sending a reply if this peer doesn't know
    // anything about the object?
    PeerMessage reply;
    reply.mutable_store_object_message()->mutable_object_id()->CopyFrom(
        requested_object_id);

    transaction_sequencer_.SendMessageToRemotePeer(
        remote_peer, reply, PeerMessageSender::NON_BLOCKING_MODE);

    return;
  }

  requested_shared_object->AddInterestedPeer(remote_peer);

  MaxVersionMap current_version_temp;
  {
    MutexLock lock(&current_sequence_point_mu_);
    current_version_temp.CopyFrom(current_sequence_point_.version_map());
  }

  PeerMessage reply;
  StoreObjectMessage* const store_object_message =
      reply.mutable_store_object_message();
  store_object_message->mutable_object_id()->CopyFrom(requested_object_id);

  map<TransactionId, unique_ptr<SharedObjectTransaction>> transactions;
  MaxVersionMap effective_version;

  requested_shared_object->GetTransactions(current_version_temp, &transactions,
                                           &effective_version);

  for (const auto& transaction_pair : transactions) {
    const TransactionId& transaction_id = transaction_pair.first;
    const SharedObjectTransaction* const transaction =
        transaction_pair.second.get();

    TransactionProto* const transaction_proto =
        store_object_message->add_transaction();
    *transaction_proto->mutable_transaction_id() = transaction_id;

    for (const unique_ptr<CommittedEvent>& event : transaction->events()) {
      ConvertCommittedEventToEventProto(event.get(),
                                        transaction_proto->add_event());
    }

    transaction_proto->set_origin_peer_id(
        transaction->origin_peer()->peer_id());
  }

  for (const auto& version_pair : effective_version.peer_transaction_ids()) {
    PeerVersion* const peer_version = store_object_message->add_peer_version();
    peer_version->set_peer_id(version_pair.first->peer_id());
    *peer_version->mutable_last_transaction_id() = version_pair.second;
  }

  unordered_set<const CanonicalPeer*> interested_peers;
  requested_shared_object->GetInterestedPeers(&interested_peers);

  for (const CanonicalPeer* const canonical_peer : interested_peers) {
    store_object_message->add_interested_peer_id(canonical_peer->peer_id());
  }

  transaction_sequencer_.SendMessageToRemotePeer(
      remote_peer, reply, PeerMessageSender::NON_BLOCKING_MODE);
}

void TransactionStore::HandleStoreObjectMessage(
    const CanonicalPeer* remote_peer,
    const StoreObjectMessage& store_object_message) {
  const Uuid& object_id = store_object_message.object_id();

  SharedObject* const shared_object = GetOrCreateSharedObject(object_id);

  map<TransactionId, unique_ptr<SharedObjectTransaction>> transactions;

  for (int i = 0; i < store_object_message.transaction_size(); ++i) {
    const TransactionProto& transaction_proto =
        store_object_message.transaction(i);

    const int event_count = transaction_proto.event_size();

    vector<unique_ptr<CommittedEvent>> events;
    events.resize(event_count);

    for (int j = 0; j < event_count; ++j) {
      const EventProto& event_proto = transaction_proto.event(j);
      events[j].reset(ConvertEventProtoToCommittedEvent(event_proto));
    }

    const CanonicalPeer* const origin_peer =
        canonical_peer_map_->GetCanonicalPeer(
            transaction_proto.origin_peer_id());

    SharedObjectTransaction* const transaction = new SharedObjectTransaction(
        events, origin_peer);

    CHECK(transactions.emplace(
        transaction_proto.transaction_id(),
        unique_ptr<SharedObjectTransaction>(transaction)).second);
  }

  MaxVersionMap version_map;

  for (int i = 0; i < store_object_message.peer_version_size(); ++i) {
    const PeerVersion& peer_version = store_object_message.peer_version(i);
    const string& peer_id = peer_version.peer_id();
    const TransactionId& last_transaction_id =
        peer_version.last_transaction_id();

    version_map.AddPeerTransactionId(
        canonical_peer_map_->GetCanonicalPeer(peer_id), last_transaction_id);
  }

  vector<pair<const CanonicalPeer*, TransactionId>> all_transactions_to_reject;

  shared_object->StoreTransactions(remote_peer, transactions, version_map,
                                   &all_transactions_to_reject);

  for (int i = 0; i < store_object_message.interested_peer_id_size(); ++i) {
    const string& interested_peer_id =
        store_object_message.interested_peer_id(i);

    shared_object->AddInterestedPeer(
        canonical_peer_map_->GetCanonicalPeer(interested_peer_id));
  }

  // TODO(dss): The following code is duplicated several places in this file.
  // Extract it out into a method.
  TransactionId new_transaction_id;
  transaction_sequencer_.ReserveTransaction(&new_transaction_id);

  RejectTransactionsAndSendMessages(all_transactions_to_reject,
                                    new_transaction_id);

  transaction_sequencer_.ReleaseTransaction(new_transaction_id);

  {
    MutexLock lock(&current_sequence_point_mu_);
    IncrementVersionNumber_Locked();
  }
}

void TransactionStore::HandleRejectTransactionMessage(
    const CanonicalPeer* remote_peer,
    const RejectTransactionMessage& reject_transaction_message) {
  const TransactionId& remote_transaction_id =
      reject_transaction_message.new_transaction_id();

  const int rejected_peer_count =
      reject_transaction_message.rejected_peer_size();

  vector<pair<const CanonicalPeer*, TransactionId>> transactions_to_reject;
  transactions_to_reject.reserve(rejected_peer_count);

  for (int rejected_peer_index = 0; rejected_peer_index < rejected_peer_count;
       ++rejected_peer_index) {
    const RejectedPeerProto& rejected_peer_proto =
        reject_transaction_message.rejected_peer(rejected_peer_index);

    const string& rejected_peer_id = rejected_peer_proto.rejected_peer_id();
    const TransactionId& rejected_transaction_id =
        rejected_peer_proto.rejected_transaction_id();

    const CanonicalPeer* const rejected_peer =
        canonical_peer_map_->GetCanonicalPeer(rejected_peer_id);

    transactions_to_reject.emplace_back(rejected_peer, rejected_transaction_id);
  }

  TransactionId new_transaction_id;
  transaction_sequencer_.ReserveTransaction(&new_transaction_id);

  RejectTransactionMessage dummy;
  RejectTransactions(transactions_to_reject, new_transaction_id, &dummy);

  transaction_sequencer_.ReleaseTransaction(new_transaction_id);

  UpdateCurrentSequencePoint(remote_peer, remote_transaction_id);
  UpdateCurrentSequencePoint(local_peer_, new_transaction_id);
}

void TransactionStore::HandleInvalidateTransactionsMessage(
    const CanonicalPeer* remote_peer,
    const InvalidateTransactionsMessage& invalidate_transactions_message) {
  const TransactionId& start_transaction_id =
      invalidate_transactions_message.start_transaction_id();
  const TransactionId& end_transaction_id =
      invalidate_transactions_message.end_transaction_id();

  {
    MutexLock lock(&current_sequence_point_mu_);
    current_sequence_point_.AddInvalidatedRange(remote_peer,
                                                start_transaction_id,
                                                end_transaction_id);
    IncrementVersionNumber_Locked();
  }

  UpdateCurrentSequencePoint(remote_peer, end_transaction_id);
}

SharedObject* TransactionStore::GetSharedObject(const Uuid& object_id) const {
  MutexLock lock(&shared_objects_mu_);

  const SharedObjectMap::const_iterator it = shared_objects_.find(object_id);
  if (it == shared_objects_.end()) {
    return nullptr;
  }

  return it->second.get();
}

SharedObject* TransactionStore::GetOrCreateSharedObject(const Uuid& object_id) {
  MutexLock lock(&shared_objects_mu_);

  unique_ptr<SharedObject>& shared_object = shared_objects_[object_id];
  if (shared_object.get() == nullptr) {
    shared_object.reset(new SharedObject(this, object_id));
  }

  return shared_object.get();
}

shared_ptr<const LiveObject>
TransactionStore::GetLiveObjectAtSequencePoint_Helper(
    SharedObject* shared_object,
    const SequencePointImpl& sequence_point_impl,
    uint64* current_version_number,
    vector<pair<const CanonicalPeer*, TransactionId>>*
        all_transactions_to_reject) {
  CHECK(shared_object != nullptr);
  CHECK(current_version_number != nullptr);
  CHECK(all_transactions_to_reject != nullptr);

  MaxVersionMap current_version_map;
  {
    MutexLock lock(&current_sequence_point_mu_);

    while (version_number_ == *current_version_number) {
      version_number_changed_cond_.Wait(&current_sequence_point_mu_);
    }

    current_version_map.CopyFrom(current_sequence_point_.version_map());
    *current_version_number = version_number_;
  }

  VLOG(4) << "Transaction store version: "
          << GetJsonString(current_version_map);
  VLOG(4) << "Sequence point: " << GetJsonString(sequence_point_impl);

  vector<pair<const CanonicalPeer*, TransactionId>> transactions_to_reject;
  const shared_ptr<const LiveObject> live_object =
      shared_object->GetWorkingVersion(current_version_map, sequence_point_impl,
                                       &transactions_to_reject);

  all_transactions_to_reject->insert(all_transactions_to_reject->end(),
                                     transactions_to_reject.begin(),
                                     transactions_to_reject.end());

  return live_object;
}

void TransactionStore::ApplyTransactionAndSendMessage(
    const TransactionId& transaction_id,
    const unordered_map<SharedObject*, unique_ptr<SharedObjectTransaction>>&
        shared_object_transactions) {
  PeerMessage peer_message;
  ApplyTransactionMessage* const apply_transaction_message =
      peer_message.mutable_apply_transaction_message();
  *apply_transaction_message->mutable_transaction_id() = transaction_id;

  unordered_set<SharedObject*> affected_objects;

  for (const auto& transaction_pair : shared_object_transactions) {
    SharedObject* const shared_object = transaction_pair.first;
    const SharedObjectTransaction* const transaction =
        transaction_pair.second.get();

    CHECK_EQ(transaction->origin_peer(), local_peer_);

    ObjectTransactionProto* const object_transaction =
        apply_transaction_message->add_object_transaction();
    object_transaction->mutable_object_id()->CopyFrom(
        shared_object->object_id());

    for (const unique_ptr<CommittedEvent>& event : transaction->events()) {
      ConvertCommittedEventToEventProto(event.get(),
                                        object_transaction->add_event());
    }

    CHECK(affected_objects.insert(shared_object).second);
  }

  ApplyTransaction(transaction_id, local_peer_, shared_object_transactions);

  SendMessageToAffectedPeers(peer_message, affected_objects);
}

void TransactionStore::ApplyTransaction(
    const TransactionId& transaction_id,
    const CanonicalPeer* origin_peer,
    const unordered_map<SharedObject*, unique_ptr<SharedObjectTransaction>>&
        shared_object_transactions) {
  CHECK(origin_peer != nullptr);

  // TODO(dss): Make sure that the transaction has a later timestamp than the
  // previous transaction received from the same originating peer.

  vector<pair<const CanonicalPeer*, TransactionId>> all_transactions_to_reject;

  for (const auto& transaction_pair : shared_object_transactions) {
    SharedObject* const shared_object = transaction_pair.first;
    const SharedObjectTransaction* const shared_object_transaction =
        transaction_pair.second.get();

    CHECK_EQ(shared_object_transaction->origin_peer(), origin_peer);

    vector<pair<const CanonicalPeer*, TransactionId>> transactions_to_reject;
    shared_object->InsertTransaction(origin_peer, transaction_id,
                                     shared_object_transaction->events(),
                                     origin_peer == local_peer_,
                                     &transactions_to_reject);

    all_transactions_to_reject.insert(all_transactions_to_reject.end(),
                                      transactions_to_reject.begin(),
                                      transactions_to_reject.end());
  }

  TransactionId new_transaction_id;
  transaction_sequencer_.ReserveTransaction(&new_transaction_id);

  RejectTransactionsAndSendMessages(all_transactions_to_reject,
                                    new_transaction_id);

  transaction_sequencer_.ReleaseTransaction(new_transaction_id);

  UpdateCurrentSequencePoint(local_peer_, new_transaction_id);
}

void TransactionStore::RejectTransactionsAndSendMessages(
    const vector<pair<const CanonicalPeer*, TransactionId>>&
        transactions_to_reject,
    const TransactionId& new_transaction_id) {
  PeerMessage peer_message;
  RejectTransactionMessage* const reject_transaction_message =
      peer_message.mutable_reject_transaction_message();

  RejectTransactions(transactions_to_reject, new_transaction_id,
                     reject_transaction_message);

  if (reject_transaction_message->rejected_peer_size() > 0) {
    transaction_sequencer_.BroadcastMessage(peer_message,
                                            PeerMessageSender::BLOCKING_MODE);
  }
}

void TransactionStore::RejectTransactions(
    const vector<pair<const CanonicalPeer*, TransactionId>>&
        transactions_to_reject,
    const TransactionId& new_transaction_id,
    RejectTransactionMessage* reject_transaction_message) {
  CHECK(reject_transaction_message != nullptr);

  *reject_transaction_message->mutable_new_transaction_id() =
      new_transaction_id;

  // Update the current sequence point.
  {
    MutexLock lock(&current_sequence_point_mu_);

    for (const auto& rejected_transaction_pair : transactions_to_reject) {
      const CanonicalPeer* const rejected_peer =
          rejected_transaction_pair.first;
      const TransactionId& rejected_transaction_id =
          rejected_transaction_pair.second;

      if (rejected_peer == local_peer_) {
        current_sequence_point_.AddInvalidatedRange(rejected_peer,
                                                    rejected_transaction_id,
                                                    new_transaction_id);
      } else {
        current_sequence_point_.AddRejectedPeer(rejected_peer,
                                                rejected_transaction_id);
      }
    }

    IncrementVersionNumber_Locked();
  }

  TransactionId invalidate_start_transaction_id = MAX_TRANSACTION_ID;

  for (const auto& rejected_transaction_pair : transactions_to_reject) {
    const CanonicalPeer* const rejected_peer = rejected_transaction_pair.first;
    const TransactionId& rejected_transaction_id =
        rejected_transaction_pair.second;

    if (rejected_peer == local_peer_) {
      if (rejected_transaction_id < invalidate_start_transaction_id) {
        invalidate_start_transaction_id = rejected_transaction_id;
      }
    } else {
      RejectedPeerProto* const rejected_peer_proto =
          reject_transaction_message->add_rejected_peer();

      rejected_peer_proto->set_rejected_peer_id(rejected_peer->peer_id());
      *rejected_peer_proto->mutable_rejected_transaction_id() =
          rejected_transaction_id;
    }
  }

  if (invalidate_start_transaction_id < MAX_TRANSACTION_ID) {
    {
      MutexLock lock(&rejected_transaction_id_mu_);
      rejected_transaction_id_ = invalidate_start_transaction_id;
      rewinding_cond_.Broadcast();
    }

    PeerMessage peer_message;
    InvalidateTransactionsMessage* const invalidate_transactions_message =
        peer_message.mutable_invalidate_transactions_message();

    *invalidate_transactions_message->mutable_start_transaction_id() =
        invalidate_start_transaction_id;
    *invalidate_transactions_message->mutable_end_transaction_id() =
        new_transaction_id;

    transaction_sequencer_.BroadcastMessage(peer_message,
                                            PeerMessageSender::BLOCKING_MODE);
  }
}

void TransactionStore::SendMessageToAffectedPeers(
    const PeerMessage& peer_message,
    const unordered_set<SharedObject*>& affected_objects) {
  unordered_set<const CanonicalPeer*> all_interested_peers;

  for (const SharedObject* const shared_object : affected_objects) {
    unordered_set<const CanonicalPeer*> interested_peers;
    shared_object->GetInterestedPeers(&interested_peers);

    all_interested_peers.insert(interested_peers.begin(),
                                interested_peers.end());
  }

  all_interested_peers.erase(local_peer_);

  for (const CanonicalPeer* const interested_peer : all_interested_peers) {
    transaction_sequencer_.SendMessageToRemotePeer(
        interested_peer, peer_message, PeerMessageSender::BLOCKING_MODE);
  }
}

void TransactionStore::UpdateCurrentSequencePoint(
    const CanonicalPeer* origin_peer, const TransactionId& transaction_id) {
  MutexLock lock(&current_sequence_point_mu_);
  current_sequence_point_.AddPeerTransactionId(origin_peer, transaction_id);
  IncrementVersionNumber_Locked();
}

void TransactionStore::IncrementVersionNumber_Locked() {
  ++version_number_;
  version_number_changed_cond_.Broadcast();
}

SharedObject* TransactionStore::GetSharedObjectForObjectReference(
    ObjectReferenceImpl* object_reference) {
  if (object_reference == nullptr) {
    return nullptr;
  }

  return object_reference->shared_object();
}

void TransactionStore::EnsureSharedObjectsInTransactionExist(
    const SharedObjectTransaction* transaction) {
  CHECK(transaction != nullptr);

  for (const unique_ptr<CommittedEvent>& event : transaction->events()) {
    EnsureSharedObjectsInEventExist(event.get());
  }
}

void TransactionStore::EnsureSharedObjectsInEventExist(
    const CommittedEvent* event) {
  const CommittedEvent::Type type = event->type();

  switch (type) {
    case CommittedEvent::OBJECT_CREATION:
    case CommittedEvent::BEGIN_TRANSACTION:
    case CommittedEvent::END_TRANSACTION:
      break;

    case CommittedEvent::SUB_OBJECT_CREATION: {
      const string* new_object_name = nullptr;
      ObjectReferenceImpl* new_object = nullptr;

      event->GetSubObjectCreation(&new_object_name, &new_object);

      GetSharedObjectForObjectReference(new_object);

      break;
    }

    case CommittedEvent::METHOD_CALL: {
      const string* method_name = nullptr;
      const vector<Value>* parameters = nullptr;

      event->GetMethodCall(&method_name, &parameters);

      for (const Value& parameter : *parameters) {
        EnsureSharedObjectInValueExists(parameter);
      }

      break;
    }

    case CommittedEvent::METHOD_RETURN: {
      const Value* return_value = nullptr;
      event->GetMethodReturn(&return_value);

      EnsureSharedObjectInValueExists(*return_value);

      break;
    }

    case CommittedEvent::SUB_METHOD_CALL: {
      ObjectReferenceImpl* callee = nullptr;
      const string* method_name = nullptr;
      const vector<Value>* parameters = nullptr;

      event->GetSubMethodCall(&callee, &method_name, &parameters);

      GetSharedObjectForObjectReference(callee);
      for (const Value& parameter : *parameters) {
        EnsureSharedObjectInValueExists(parameter);
      }

      break;
    }

    case CommittedEvent::SUB_METHOD_RETURN: {
      const Value* return_value = nullptr;
      event->GetSubMethodReturn(&return_value);

      EnsureSharedObjectInValueExists(*return_value);

      break;
    }

    case CommittedEvent::SELF_METHOD_CALL: {
      const string* method_name = nullptr;
      const vector<Value>* parameters = nullptr;

      event->GetSelfMethodCall(&method_name, &parameters);

      for (const Value& parameter : *parameters) {
        EnsureSharedObjectInValueExists(parameter);
      }

      break;
    }

    case CommittedEvent::SELF_METHOD_RETURN: {
      const Value* return_value = nullptr;
      event->GetSelfMethodReturn(&return_value);

      EnsureSharedObjectInValueExists(*return_value);

      break;
    }

    default:
      LOG(FATAL) << "Invalid committed event type: " << static_cast<int>(type);
  }
}

void TransactionStore::EnsureSharedObjectInValueExists(const Value& value) {
  if (value.type() == Value::OBJECT_REFERENCE) {
    ObjectReferenceImpl* const object_reference =
        static_cast<ObjectReferenceImpl*>(value.object_reference());
    GetSharedObjectForObjectReference(object_reference);
  }
}

void TransactionStore::ConvertCommittedEventToEventProto(
    const CommittedEvent* in, EventProto* out) {
  CHECK(in != nullptr);
  CHECK(out != nullptr);

  const CommittedEvent::Type type = in->type();

  switch (type) {
    case CommittedEvent::OBJECT_CREATION: {
      shared_ptr<const LiveObject> live_object;
      in->GetObjectCreation(&live_object);

      ObjectCreationEventProto* const object_creation_event_proto =
          out->mutable_object_creation();

      vector<ObjectReferenceImpl*> object_references;
      live_object->Serialize(object_creation_event_proto->mutable_data(),
                             &object_references);

      for (ObjectReferenceImpl* const object_reference : object_references) {
        SharedObject* const shared_object = GetSharedObjectForObjectReference(
            object_reference);
        object_creation_event_proto->add_referenced_object_id()->CopyFrom(
            shared_object->object_id());
      }
      break;
    }

    case CommittedEvent::SUB_OBJECT_CREATION: {
      const string* new_object_name = nullptr;
      ObjectReferenceImpl* new_object = nullptr;

      in->GetSubObjectCreation(&new_object_name, &new_object);

      SubObjectCreationEventProto* const sub_object_creation_event_proto =
          out->mutable_sub_object_creation();

      sub_object_creation_event_proto->set_new_object_name(*new_object_name);

      const SharedObject* const shared_object =
          GetSharedObjectForObjectReference(new_object);
      sub_object_creation_event_proto->mutable_new_object_id()->CopyFrom(
          shared_object->object_id());

      break;
    }

    case CommittedEvent::BEGIN_TRANSACTION:
      out->mutable_begin_transaction();
      break;

    case CommittedEvent::END_TRANSACTION:
      out->mutable_end_transaction();
      break;

    case CommittedEvent::METHOD_CALL: {
      const string* method_name = nullptr;
      const vector<Value>* parameters = nullptr;

      in->GetMethodCall(&method_name, &parameters);

      MethodCallEventProto* const method_call_event_proto =
          out->mutable_method_call();
      method_call_event_proto->set_method_name(*method_name);

      for (const Value& parameter : *parameters) {
        ConvertValueToValueProto(parameter,
                                 method_call_event_proto->add_parameter());
      }

      break;
    }

    case CommittedEvent::METHOD_RETURN: {
      const Value* return_value = nullptr;

      in->GetMethodReturn(&return_value);

      MethodReturnEventProto* const method_return_event_proto =
          out->mutable_method_return();
      ConvertValueToValueProto(
          *return_value, method_return_event_proto->mutable_return_value());

      break;
    }

    case CommittedEvent::SUB_METHOD_CALL: {
      ObjectReferenceImpl* callee = nullptr;
      const string* method_name = nullptr;
      const vector<Value>* parameters = nullptr;

      in->GetSubMethodCall(&callee, &method_name, &parameters);

      SubMethodCallEventProto* const sub_method_call_event_proto =
          out->mutable_sub_method_call();
      sub_method_call_event_proto->set_method_name(*method_name);

      for (const Value& parameter : *parameters) {
        ConvertValueToValueProto(parameter,
                                 sub_method_call_event_proto->add_parameter());
      }

      const SharedObject* const callee_shared_object =
          GetSharedObjectForObjectReference(callee);
      CHECK(callee_shared_object != nullptr);
      sub_method_call_event_proto->mutable_callee_object_id()->CopyFrom(
          callee_shared_object->object_id());

      break;
    }

    case CommittedEvent::SUB_METHOD_RETURN: {
      const Value* return_value = nullptr;

      in->GetSubMethodReturn(&return_value);

      SubMethodReturnEventProto* const sub_method_return_event_proto =
          out->mutable_sub_method_return();
      ConvertValueToValueProto(
          *return_value, sub_method_return_event_proto->mutable_return_value());

      break;
    }

    case CommittedEvent::SELF_METHOD_CALL: {
      const string* method_name = nullptr;
      const vector<Value>* parameters = nullptr;

      in->GetSelfMethodCall(&method_name, &parameters);

      SelfMethodCallEventProto* const self_method_call_event_proto =
          out->mutable_self_method_call();
      self_method_call_event_proto->set_method_name(*method_name);

      for (const Value& parameter : *parameters) {
        ConvertValueToValueProto(parameter,
                                 self_method_call_event_proto->add_parameter());
      }

      break;
    }

    case CommittedEvent::SELF_METHOD_RETURN: {
      const Value* return_value = nullptr;

      in->GetSelfMethodReturn(&return_value);

      SelfMethodReturnEventProto* const self_method_return_event_proto =
          out->mutable_self_method_return();
      ConvertValueToValueProto(
          *return_value,
          self_method_return_event_proto->mutable_return_value());

      break;
    }

    default:
      LOG(FATAL) << "Invalid committed event type: " << static_cast<int>(type);
  }
}

CommittedEvent* TransactionStore::ConvertEventProtoToCommittedEvent(
    const EventProto& event_proto) {
  const EventProto::Type type = GetEventProtoType(event_proto);

  switch (type) {
    case EventProto::OBJECT_CREATION: {
      const ObjectCreationEventProto& object_creation_event_proto =
          event_proto.object_creation();

      const int referenced_object_count =
          object_creation_event_proto.referenced_object_id_size();
      vector<ObjectReferenceImpl*> object_references(referenced_object_count);

      for (int i = 0; i < referenced_object_count; ++i) {
        const Uuid& object_id =
            object_creation_event_proto.referenced_object_id(i);
        SharedObject* const referenced_shared_object = GetOrCreateSharedObject(
            object_id);
        object_references[i] =
            referenced_shared_object->GetOrCreateObjectReference();
      }

      const shared_ptr<const LiveObject> live_object(
          new LiveObject(
              DeserializeLocalObjectFromString(
                  interpreter_, object_creation_event_proto.data(),
                  object_references)));

      return new ObjectCreationCommittedEvent(live_object);
    }

    case EventProto::SUB_OBJECT_CREATION: {
      const SubObjectCreationEventProto& sub_object_creation_event_proto =
          event_proto.sub_object_creation();
      const Uuid& object_id = sub_object_creation_event_proto.new_object_id();
      SharedObject* const shared_object = GetOrCreateSharedObject(object_id);

      return new SubObjectCreationCommittedEvent(
          sub_object_creation_event_proto.new_object_name(),
          shared_object->GetOrCreateObjectReference());
    }

    case EventProto::BEGIN_TRANSACTION:
      return new BeginTransactionCommittedEvent();

    case EventProto::END_TRANSACTION:
      return new EndTransactionCommittedEvent();

    case EventProto::METHOD_CALL: {
      const MethodCallEventProto& method_call_event_proto =
          event_proto.method_call();

      const string& method_name = method_call_event_proto.method_name();

      const int parameter_count = method_call_event_proto.parameter_size();
      vector<Value> parameters(parameter_count);

      for (int i = 0; i < parameter_count; ++i) {
        ConvertValueProtoToValue(method_call_event_proto.parameter(i),
                                 &parameters[i]);
      }

      return new MethodCallCommittedEvent(method_name, parameters);
    }

    case EventProto::METHOD_RETURN: {
      const MethodReturnEventProto& method_return_event_proto =
          event_proto.method_return();

      Value return_value;
      ConvertValueProtoToValue(method_return_event_proto.return_value(),
                               &return_value);

      return new MethodReturnCommittedEvent(return_value);
    }

    case EventProto::SUB_METHOD_CALL: {
      const SubMethodCallEventProto& sub_method_call_event_proto =
          event_proto.sub_method_call();

      SharedObject* const callee = GetOrCreateSharedObject(
          sub_method_call_event_proto.callee_object_id());
      const string& method_name = sub_method_call_event_proto.method_name();

      const int parameter_count = sub_method_call_event_proto.parameter_size();
      vector<Value> parameters(parameter_count);

      for (int i = 0; i < parameter_count; ++i) {
        ConvertValueProtoToValue(sub_method_call_event_proto.parameter(i),
                                 &parameters[i]);
      }

      return new SubMethodCallCommittedEvent(
          callee->GetOrCreateObjectReference(), method_name, parameters);
    }

    case EventProto::SUB_METHOD_RETURN: {
      const SubMethodReturnEventProto& sub_method_return_event_proto =
          event_proto.sub_method_return();

      Value return_value;
      ConvertValueProtoToValue(sub_method_return_event_proto.return_value(),
                               &return_value);

      return new SubMethodReturnCommittedEvent(return_value);
    }

    case EventProto::SELF_METHOD_CALL: {
      const SelfMethodCallEventProto& self_method_call_event_proto =
          event_proto.self_method_call();

      const string& method_name = self_method_call_event_proto.method_name();

      const int parameter_count = self_method_call_event_proto.parameter_size();
      vector<Value> parameters(parameter_count);

      for (int i = 0; i < parameter_count; ++i) {
        ConvertValueProtoToValue(self_method_call_event_proto.parameter(i),
                                 &parameters[i]);
      }

      return new SelfMethodCallCommittedEvent(method_name, parameters);
    }

    case EventProto::SELF_METHOD_RETURN: {
      const SelfMethodReturnEventProto& self_method_return_event_proto =
          event_proto.self_method_return();

      Value return_value;
      ConvertValueProtoToValue(self_method_return_event_proto.return_value(),
                               &return_value);

      return new SelfMethodReturnCommittedEvent(return_value);
    }

    default:
      LOG(FATAL) << "Invalid event type: " << static_cast<int>(type);
  }
}

#define CONVERT_VALUE(enum_const, setter_method, getter_method) \
  case ValueProto::enum_const: \
    out->setter_method(local_type, in.getter_method()); \
    break;

void TransactionStore::ConvertValueProtoToValue(const ValueProto& in,
                                                Value* out) {
  CHECK(out != nullptr);

  const int local_type = in.local_type();
  const ValueProto::Type type = GetValueProtoType(in);

  switch (type) {
    case ValueProto::EMPTY:
      out->set_empty(local_type);
      break;

    CONVERT_VALUE(DOUBLE, set_double_value, double_value);
    CONVERT_VALUE(FLOAT, set_float_value, float_value);
    CONVERT_VALUE(INT64, set_int64_value, int64_value);
    CONVERT_VALUE(UINT64, set_uint64_value, uint64_value);
    CONVERT_VALUE(BOOL, set_bool_value, bool_value);
    CONVERT_VALUE(STRING, set_string_value, string_value);
    CONVERT_VALUE(BYTES, set_bytes_value, bytes_value);

    case ValueProto::OBJECT_ID: {
      SharedObject* const shared_object =
          GetOrCreateSharedObject(in.object_id());
      out->set_object_reference(local_type,
                                shared_object->GetOrCreateObjectReference());
      break;
    }

    default:
      LOG(FATAL) << "Unexpected value proto type: " << static_cast<int>(type);
  }
}

#undef CONVERT_VALUE

}  // namespace engine
}  // namespace floating_temple
