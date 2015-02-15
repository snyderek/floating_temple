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
#include <map>
#include <string>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include <utility>
#include <vector>

#include "base/cond_var.h"
#include "base/escape.h"
#include "base/integral_types.h"
#include "base/linked_ptr.h"
#include "base/logging.h"
#include "base/mutex.h"
#include "base/mutex_lock.h"
#include "include/c++/value.h"
#include "peer/canonical_peer.h"
#include "peer/canonical_peer_map.h"
#include "peer/committed_event.h"
#include "peer/committed_value.h"
#include "peer/const_live_object_ptr.h"
#include "peer/convert_value.h"
#include "peer/get_event_proto_type.h"
#include "peer/get_peer_message_type.h"
#include "peer/interpreter_thread.h"
#include "peer/live_object.h"
#include "peer/live_object_ptr.h"
#include "peer/max_version_map.h"
#include "peer/min_version_map.h"
#include "peer/peer_message_sender.h"
#include "peer/peer_object_impl.h"
#include "peer/pending_event.h"
#include "peer/proto/event.pb.h"
#include "peer/proto/peer.pb.h"
#include "peer/proto/transaction_id.pb.h"
#include "peer/proto/uuid.pb.h"
#include "peer/proto/value_proto.pb.h"
#include "peer/sequence_point_impl.h"
#include "peer/serialize_local_object_to_string.h"
#include "peer/shared_object.h"
#include "peer/shared_object_transaction_info.h"
#include "peer/transaction_id_generator.h"
#include "peer/transaction_id_util.h"
#include "peer/transaction_sequencer.h"
#include "peer/uuid_util.h"
#include "peer/value_proto_util.h"

using std::make_pair;
using std::map;
using std::pair;
using std::size_t;
using std::string;
using std::tr1::unordered_map;
using std::tr1::unordered_set;
using std::vector;

namespace floating_temple {
namespace peer {

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
      version_number_(1) {
  TransactionId initial_transaction_id;
  transaction_id_generator_.Generate(&initial_transaction_id);

  current_sequence_point_.AddPeerTransactionId(local_peer,
                                               initial_transaction_id);
}

TransactionStore::~TransactionStore() {
}

Thread* TransactionStore::CreateThread() {
  InterpreterThread* const thread = new InterpreterThread(this);

  {
    MutexLock lock(&interpreter_threads_mu_);
    interpreter_threads_.push_back(make_linked_ptr(thread));
  }

  return thread;
}

void TransactionStore::NotifyNewConnection(const CanonicalPeer* remote_peer) {
  unordered_set<SharedObject*> named_objects;
  {
    MutexLock lock(&named_objects_mu_);
    named_objects = named_objects_;
  }

  for (unordered_set<SharedObject*>::const_iterator it = named_objects.begin();
       it != named_objects.end(); ++it) {
    const SharedObject* const shared_object = *it;

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
  CHECK(remote_peer != NULL);

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

SequencePoint* TransactionStore::GetCurrentSequencePoint() const {
  MutexLock lock(&current_sequence_point_mu_);
  return current_sequence_point_.Clone();
}

ConstLiveObjectPtr TransactionStore::GetLiveObjectAtSequencePoint(
    PeerObjectImpl* peer_object, const SequencePoint* sequence_point,
    bool wait) {
  CHECK(peer_object != NULL);

  SharedObject* const shared_object = peer_object->shared_object();
  // The peer object must have been created by a committed transaction, because
  // otherwise the pending transaction wouldn't need to request it. Therefore a
  // shared object should exist for the peer object.
  CHECK(shared_object != NULL);

  const SequencePointImpl* const sequence_point_impl =
      static_cast<const SequencePointImpl*>(sequence_point);

  uint64 current_version_number = 0;
  unordered_map<SharedObject*, PeerObjectImpl*> new_peer_objects;
  vector<pair<const CanonicalPeer*, TransactionId> > all_transactions_to_reject;

  ConstLiveObjectPtr live_object = GetLiveObjectAtSequencePoint_Helper(
      shared_object, *sequence_point_impl, &current_version_number,
      &new_peer_objects, &all_transactions_to_reject);

  if (live_object.get() == NULL) {
    PeerMessage peer_message;
    GetObjectMessage* const get_object_message =
        peer_message.mutable_get_object_message();
    get_object_message->mutable_object_id()->CopyFrom(
        shared_object->object_id());

    transaction_sequencer_.BroadcastMessage(peer_message,
                                            PeerMessageSender::BLOCKING_MODE);

    if (wait) {
      while (live_object.get() == NULL) {
        live_object = GetLiveObjectAtSequencePoint_Helper(
            shared_object, *sequence_point_impl, &current_version_number,
            &new_peer_objects, &all_transactions_to_reject);
      }
    }
  }

  TransactionId new_transaction_id;
  transaction_sequencer_.ReserveTransaction(&new_transaction_id);

  RejectTransactionsAndSendMessages(all_transactions_to_reject,
                                    new_transaction_id);

  transaction_sequencer_.ReleaseTransaction(new_transaction_id);

  UpdateCurrentSequencePoint(local_peer_, new_transaction_id);

  CreateNewPeerObjects(new_peer_objects);

  return live_object;
}

PeerObjectImpl* TransactionStore::CreateUnboundPeerObject() {
  PeerObjectImpl* const peer_object = new PeerObjectImpl();
  {
    MutexLock lock(&peer_objects_mu_);
    // TODO(dss): Garbage-collect PeerObjectImpl instances when they're no
    // longer being used.
    peer_objects_.push_back(make_linked_ptr(peer_object));
  }

  return peer_object;
}

PeerObjectImpl* TransactionStore::GetOrCreateNamedObject(const string& name) {
  CHECK(!name.empty());

  Uuid object_id;
  GeneratePredictableUuid(object_namespace_uuid_, name, &object_id);

  SharedObject* const shared_object = GetOrCreateSharedObject(object_id);

  {
    MutexLock lock(&named_objects_mu_);
    named_objects_.insert(shared_object);
  }

  return shared_object->GetOrCreatePeerObject();
}

void TransactionStore::CreateTransaction(
    const vector<linked_ptr<PendingEvent> >& events,
    TransactionId* transaction_id,
    const unordered_map<PeerObjectImpl*, LiveObjectPtr>& modified_objects,
    const SequencePoint* prev_sequence_point) {
  CHECK(transaction_id != NULL);
  CHECK(prev_sequence_point != NULL);

  TransactionId transaction_id_temp;
  transaction_sequencer_.ReserveTransaction(&transaction_id_temp);

  const vector<linked_ptr<PendingEvent> >::size_type event_count =
      events.size();

  VLOG(2) << "Creating local transaction "
          << TransactionIdToString(transaction_id_temp) << " with "
          << event_count << " events.";

  if (VLOG_IS_ON(3)) {
    for (vector<linked_ptr<PendingEvent> >::size_type i = 0; i < event_count;
         ++i) {
      const PendingEvent* const event = events[i].get();
      const PendingEvent::Type type = event->type();

      switch (type) {
        case PendingEvent::BEGIN_TRANSACTION:
          VLOG(3) << "Event " << i << ": BEGIN_TRANSACTION";
          break;

        case PendingEvent::END_TRANSACTION:
          VLOG(3) << "Event " << i << ": END_TRANSACTION";
          break;

        case PendingEvent::METHOD_CALL: {
          PeerObjectImpl* next_peer_object = NULL;
          const string* method_name = NULL;
          const vector<Value>* parameters = NULL;

          event->GetMethodCall(&next_peer_object, &method_name, &parameters);

          VLOG(3) << "Event " << i << ": METHOD_CALL \""
                  << CEscape(*method_name) << "\"";

          break;
        }

        case PendingEvent::METHOD_RETURN:
          VLOG(3) << "Event " << i << ": METHOD_RETURN";
          break;

        default:
          LOG(FATAL) << "Invalid pending event type: "
                     << static_cast<int>(type);
      }
    }
  }

  unordered_map<SharedObject*, linked_ptr<SharedObjectTransactionInfo> >
      shared_object_transactions;

  for (vector<linked_ptr<PendingEvent> >::size_type i = 0; i < event_count;
       ++i) {
    ConvertPendingEventToCommittedEvents(events[i].get(), local_peer_,
                                         &shared_object_transactions);
  }

  ApplyTransactionAndSendMessage(transaction_id_temp,
                                 &shared_object_transactions);

  transaction_sequencer_.ReleaseTransaction(transaction_id_temp);

  SequencePointImpl cached_version_sequence_point;
  cached_version_sequence_point.CopyFrom(
      *static_cast<const SequencePointImpl*>(prev_sequence_point));
  cached_version_sequence_point.AddPeerTransactionId(local_peer_,
                                                     transaction_id_temp);

  for (unordered_map<PeerObjectImpl*, LiveObjectPtr>::const_iterator it =
           modified_objects.begin();
       it != modified_objects.end(); ++it) {
    PeerObjectImpl* const peer_object = it->first;
    const ConstLiveObjectPtr live_object = it->second;

    SharedObject* const shared_object = peer_object->shared_object();

    if (shared_object != NULL) {
      shared_object->SetCachedLiveObject(live_object,
                                         cached_version_sequence_point);
    }
  }

  transaction_id->Swap(&transaction_id_temp);
}

bool TransactionStore::ObjectsAreEquivalent(const PeerObjectImpl* a,
                                            const PeerObjectImpl* b) const {
  CHECK(a != NULL);
  CHECK(b != NULL);

  if (a == b) {
    return true;
  }

  const SharedObject* const a_shared_object = a->shared_object();
  const SharedObject* const b_shared_object = b->shared_object();

  return a_shared_object != NULL && a_shared_object == b_shared_object;
}

void TransactionStore::HandleApplyTransactionMessage(
    const CanonicalPeer* remote_peer,
    const ApplyTransactionMessage& apply_transaction_message) {
  CHECK(remote_peer != NULL);

  const TransactionId& transaction_id =
      apply_transaction_message.transaction_id();

  unordered_map<SharedObject*, linked_ptr<SharedObjectTransactionInfo> >
      shared_object_transactions;

  for (int i = 0; i < apply_transaction_message.object_transaction_size();
       ++i) {
    const ObjectTransactionProto& object_transaction =
        apply_transaction_message.object_transaction(i);

    SharedObject* const shared_object = GetSharedObject(
        object_transaction.object_id());

    if (shared_object != NULL) {
      const int event_count = object_transaction.event_size();

      linked_ptr<SharedObjectTransactionInfo>& transaction =
          shared_object_transactions[shared_object];
      if (transaction.get() == NULL) {
        transaction.reset(new SharedObjectTransactionInfo());
        transaction->origin_peer = remote_peer;
      }

      vector<linked_ptr<CommittedEvent> >* const events = &transaction->events;
      events->resize(event_count);

      for (int j = 0; j < event_count; ++j) {
        const EventProto& event_proto = object_transaction.event(j);
        (*events)[j].reset(ConvertEventProtoToCommittedEvent(event_proto));
      }
    }
  }

  ApplyTransaction(transaction_id, remote_peer, &shared_object_transactions);
}

void TransactionStore::HandleGetObjectMessage(
    const CanonicalPeer* remote_peer,
    const GetObjectMessage& get_object_message) {
  const Uuid& requested_object_id = get_object_message.object_id();

  SharedObject* const requested_shared_object = GetSharedObject(
      requested_object_id);

  if (requested_shared_object == NULL) {
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

  map<TransactionId, linked_ptr<SharedObjectTransactionInfo> > transactions;
  MaxVersionMap effective_version;

  requested_shared_object->GetTransactions(current_version_temp, &transactions,
                                           &effective_version);

  for (map<TransactionId, linked_ptr<SharedObjectTransactionInfo> >::
           const_iterator transaction_it = transactions.begin();
       transaction_it != transactions.end(); ++transaction_it) {
    const TransactionId& transaction_id = transaction_it->first;
    const SharedObjectTransactionInfo& transaction_info =
        *transaction_it->second;

    TransactionProto* const transaction_proto =
        store_object_message->add_transaction();
    transaction_proto->mutable_transaction_id()->CopyFrom(transaction_id);

    const vector<linked_ptr<CommittedEvent> >& events = transaction_info.events;
    for (vector<linked_ptr<CommittedEvent> >::const_iterator event_it =
             events.begin();
         event_it != events.end(); ++event_it) {
      ConvertCommittedEventToEventProto(event_it->get(),
                                        transaction_proto->add_event());
    }

    transaction_proto->set_origin_peer_id(
        transaction_info.origin_peer->peer_id());
  }

  const unordered_map<const CanonicalPeer*, TransactionId>&
      peer_last_transaction_ids = effective_version.peer_transaction_ids();

  for (unordered_map<const CanonicalPeer*, TransactionId>::const_iterator it =
           peer_last_transaction_ids.begin();
       it != peer_last_transaction_ids.end(); ++it) {
    PeerVersion* const peer_version = store_object_message->add_peer_version();
    peer_version->set_peer_id(it->first->peer_id());
    peer_version->mutable_last_transaction_id()->CopyFrom(it->second);
  }

  unordered_set<const CanonicalPeer*> interested_peers;
  requested_shared_object->GetInterestedPeers(&interested_peers);

  for (unordered_set<const CanonicalPeer*>::const_iterator it =
           interested_peers.begin();
       it != interested_peers.end(); ++it) {
    store_object_message->add_interested_peer_id((*it)->peer_id());
  }

  transaction_sequencer_.SendMessageToRemotePeer(
      remote_peer, reply, PeerMessageSender::NON_BLOCKING_MODE);
}

void TransactionStore::HandleStoreObjectMessage(
    const CanonicalPeer* remote_peer,
    const StoreObjectMessage& store_object_message) {
  const Uuid& object_id = store_object_message.object_id();

  SharedObject* const shared_object = GetOrCreateSharedObject(object_id);

  map<TransactionId, linked_ptr<SharedObjectTransactionInfo> > transactions;

  for (int i = 0; i < store_object_message.transaction_size(); ++i) {
    const TransactionProto& transaction_proto =
        store_object_message.transaction(i);
    SharedObjectTransactionInfo* const transaction_info =
        new SharedObjectTransactionInfo();

    const TransactionId& transaction_id = transaction_proto.transaction_id();

    const int event_count = transaction_proto.event_size();

    vector<linked_ptr<CommittedEvent> >* const events =
        &transaction_info->events;
    events->resize(event_count);

    for (int j = 0; j < event_count; ++j) {
      const EventProto& event_proto = transaction_proto.event(j);
      (*events)[j].reset(ConvertEventProtoToCommittedEvent(event_proto));
    }

    transaction_info->origin_peer = canonical_peer_map_->GetCanonicalPeer(
        transaction_proto.origin_peer_id());

    CHECK(transactions.insert(
              make_pair(transaction_id,
                        make_linked_ptr(transaction_info))).second);
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

  shared_object->StoreTransactions(remote_peer, &transactions, version_map);

  for (int i = 0; i < store_object_message.interested_peer_id_size(); ++i) {
    const string& interested_peer_id =
        store_object_message.interested_peer_id(i);

    shared_object->AddInterestedPeer(
        canonical_peer_map_->GetCanonicalPeer(interested_peer_id));
  }

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

  vector<pair<const CanonicalPeer*, TransactionId> > transactions_to_reject;
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

    transactions_to_reject.push_back(make_pair(rejected_peer,
                                               rejected_transaction_id));
  }

  RejectTransactionMessage dummy;
  RejectTransactions(transactions_to_reject, remote_transaction_id, &dummy);

  UpdateCurrentSequencePoint(remote_peer, remote_transaction_id);
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
    return NULL;
  }

  return it->second.get();
}

SharedObject* TransactionStore::GetOrCreateSharedObject(const Uuid& object_id) {
  MutexLock lock(&shared_objects_mu_);

  linked_ptr<SharedObject>& shared_object = shared_objects_[object_id];
  if (shared_object.get() == NULL) {
    shared_object.reset(new SharedObject(this, object_id));
  }

  return shared_object.get();
}

bool TransactionStore::GetOrCreateSharedObjectForPeerObject(
    PeerObjectImpl* peer_object, SharedObject** shared_object) {
  CHECK(peer_object != NULL);
  CHECK(shared_object != NULL);

  SharedObject* shared_object_temp = peer_object->shared_object();

  if (shared_object_temp != NULL) {
    *shared_object = shared_object_temp;
    return false;
  }

  Uuid object_id;
  GenerateUuid(&object_id);

  SharedObject* const new_shared_object = new SharedObject(this, object_id);
  new_shared_object->AddPeerObject(peer_object);

  shared_object_temp = peer_object->SetSharedObjectIfUnset(new_shared_object);

  if (shared_object_temp != new_shared_object) {
    delete new_shared_object;

    *shared_object = shared_object_temp;
    return false;
  }

  {
    MutexLock lock(&shared_objects_mu_);
    CHECK(shared_objects_.insert(
              make_pair(object_id, make_linked_ptr(new_shared_object))).second);
  }

  *shared_object = new_shared_object;
  return true;
}

ConstLiveObjectPtr TransactionStore::GetLiveObjectAtSequencePoint_Helper(
    SharedObject* shared_object,
    const SequencePointImpl& sequence_point_impl,
    uint64* current_version_number,
    unordered_map<SharedObject*, PeerObjectImpl*>* new_peer_objects,
    vector<pair<const CanonicalPeer*, TransactionId> >*
        all_transactions_to_reject) {
  CHECK(shared_object != NULL);
  CHECK(current_version_number != NULL);
  CHECK(all_transactions_to_reject != NULL);

  MaxVersionMap current_version_map;
  {
    MutexLock lock(&current_sequence_point_mu_);

    while (version_number_ == *current_version_number) {
      version_number_changed_cond_.Wait(&current_sequence_point_mu_);
    }

    current_version_map.CopyFrom(current_sequence_point_.version_map());
    *current_version_number = version_number_;
  }

  VLOG(4) << "Transaction store version: " << current_version_map.Dump();
  VLOG(4) << "Sequence point: " << sequence_point_impl.Dump();

  vector<pair<const CanonicalPeer*, TransactionId> > transactions_to_reject;
  const ConstLiveObjectPtr live_object = shared_object->GetWorkingVersion(
      current_version_map, sequence_point_impl, new_peer_objects,
      &transactions_to_reject);

  all_transactions_to_reject->insert(all_transactions_to_reject->end(),
                                     transactions_to_reject.begin(),
                                     transactions_to_reject.end());

  return live_object;
}

void TransactionStore::ApplyTransactionAndSendMessage(
    const TransactionId& transaction_id,
    unordered_map<SharedObject*, linked_ptr<SharedObjectTransactionInfo> >*
        shared_object_transactions) {
  CHECK(shared_object_transactions != NULL);

  PeerMessage peer_message;
  ApplyTransactionMessage* const apply_transaction_message =
      peer_message.mutable_apply_transaction_message();
  apply_transaction_message->mutable_transaction_id()->CopyFrom(transaction_id);

  unordered_set<SharedObject*> affected_objects;

  for (unordered_map<SharedObject*, linked_ptr<SharedObjectTransactionInfo> >::
           const_iterator shared_object_it =
           shared_object_transactions->begin();
       shared_object_it != shared_object_transactions->end();
       ++shared_object_it) {
    SharedObject* const shared_object = shared_object_it->first;
    const SharedObjectTransactionInfo& transaction = *shared_object_it->second;

    CHECK_EQ(transaction.origin_peer, local_peer_);

    ObjectTransactionProto* const object_transaction =
        apply_transaction_message->add_object_transaction();
    object_transaction->mutable_object_id()->CopyFrom(
        shared_object->object_id());

    const vector<linked_ptr<CommittedEvent> >& events = transaction.events;

    for (vector<linked_ptr<CommittedEvent> >::const_iterator event_it =
             events.begin();
         event_it != events.end(); ++event_it) {
      ConvertCommittedEventToEventProto(event_it->get(),
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
    unordered_map<SharedObject*, linked_ptr<SharedObjectTransactionInfo> >*
        shared_object_transactions) {
  CHECK(origin_peer != NULL);
  CHECK(shared_object_transactions != NULL);

  // TODO(dss): Make sure that the transaction has a later timestamp than the
  // previous transaction received from the same originating peer.

  for (unordered_map<SharedObject*, linked_ptr<SharedObjectTransactionInfo> >::
           iterator it = shared_object_transactions->begin();
       it != shared_object_transactions->end(); ++it) {
    SharedObject* const shared_object = it->first;
    SharedObjectTransactionInfo* const shared_object_transaction =
        it->second.get();

    CHECK_EQ(shared_object_transaction->origin_peer, origin_peer);

    shared_object->InsertTransaction(origin_peer, transaction_id,
                                     &shared_object_transaction->events);
  }

  UpdateCurrentSequencePoint(origin_peer, transaction_id);
}

void TransactionStore::RejectTransactionsAndSendMessages(
    const vector<pair<const CanonicalPeer*, TransactionId> >&
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
    const vector<pair<const CanonicalPeer*, TransactionId> >&
        transactions_to_reject,
    const TransactionId& new_transaction_id,
    RejectTransactionMessage* reject_transaction_message) {
  CHECK(reject_transaction_message != NULL);

  reject_transaction_message->mutable_new_transaction_id()->CopyFrom(
      new_transaction_id);

  // Update the current sequence point.
  {
    MutexLock lock(&current_sequence_point_mu_);

    for (vector<pair<const CanonicalPeer*, TransactionId> >::const_iterator it =
             transactions_to_reject.begin();
         it != transactions_to_reject.end(); ++it) {
      const CanonicalPeer* const rejected_peer = it->first;
      const TransactionId& rejected_transaction_id = it->second;

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

  TransactionId invalidate_start_transaction_id;
  GetMaxTransactionId(&invalidate_start_transaction_id);

  for (vector<pair<const CanonicalPeer*, TransactionId> >::const_iterator it =
           transactions_to_reject.begin();
       it != transactions_to_reject.end(); ++it) {
    const CanonicalPeer* const rejected_peer = it->first;
    const TransactionId& rejected_transaction_id = it->second;

    if (rejected_peer == local_peer_) {
      if (CompareTransactionIds(rejected_transaction_id,
                                invalidate_start_transaction_id) < 0) {
        invalidate_start_transaction_id.CopyFrom(rejected_transaction_id);
      }
    } else {
      RejectedPeerProto* const rejected_peer_proto =
          reject_transaction_message->add_rejected_peer();

      rejected_peer_proto->set_rejected_peer_id(rejected_peer->peer_id());
      rejected_peer_proto->mutable_rejected_transaction_id()->CopyFrom(
          rejected_transaction_id);
    }
  }

  if (IsValidTransactionId(invalidate_start_transaction_id)) {
    vector<InterpreterThread*> interpreter_threads_temp;
    {
      MutexLock lock(&interpreter_threads_mu_);

      const vector<linked_ptr<InterpreterThread> >::size_type thread_count =
          interpreter_threads_.size();
      interpreter_threads_temp.resize(thread_count);

      for (vector<linked_ptr<InterpreterThread> >::size_type i = 0;
           i < thread_count; ++i) {
        interpreter_threads_temp[i] = interpreter_threads_[i].get();
      }
    }

    // TODO(dss): There's a race condition here. If an interpreter thread is
    // created after the interpreter_threads_ collection is copied, then
    // execution will not be suspended on the new thread as it should be.
    //
    // This is not currently a problem, because only one interpreter thread is
    // created per peer, and the remote peers should have no reason to reject
    // the local peer's transactions until after the interpreter thread has
    // started executing. Nonetheless, it would nice to fix the race condition.

    for (vector<InterpreterThread*>::const_iterator it =
             interpreter_threads_temp.begin();
         it != interpreter_threads_temp.end(); ++it) {
      (*it)->Rewind(invalidate_start_transaction_id);
    }

    for (vector<InterpreterThread*>::const_iterator it =
             interpreter_threads_temp.begin();
         it != interpreter_threads_temp.end(); ++it) {
      (*it)->Resume();
    }

    PeerMessage peer_message;
    InvalidateTransactionsMessage* const invalidate_transactions_message =
        peer_message.mutable_invalidate_transactions_message();

    invalidate_transactions_message->mutable_start_transaction_id()->CopyFrom(
        invalidate_start_transaction_id);
    invalidate_transactions_message->mutable_end_transaction_id()->CopyFrom(
        new_transaction_id);

    transaction_sequencer_.BroadcastMessage(peer_message,
                                            PeerMessageSender::BLOCKING_MODE);
  }
}

void TransactionStore::SendMessageToAffectedPeers(
    const PeerMessage& peer_message,
    const unordered_set<SharedObject*>& affected_objects) {
  unordered_set<const CanonicalPeer*> all_interested_peers;

  for (unordered_set<SharedObject*>::const_iterator it =
           affected_objects.begin();
       it != affected_objects.end(); ++it) {
    const SharedObject* const shared_object = *it;
    unordered_set<const CanonicalPeer*> interested_peers;
    shared_object->GetInterestedPeers(&interested_peers);

    all_interested_peers.insert(interested_peers.begin(),
                                interested_peers.end());
  }

  all_interested_peers.erase(local_peer_);

  for (unordered_set<const CanonicalPeer*>::const_iterator it =
           all_interested_peers.begin();
       it != all_interested_peers.end(); ++it) {
    transaction_sequencer_.SendMessageToRemotePeer(
        *it, peer_message, PeerMessageSender::BLOCKING_MODE);
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

void TransactionStore::CreateNewPeerObjects(
    const unordered_map<SharedObject*, PeerObjectImpl*>& new_peer_objects) {
  for (unordered_map<SharedObject*, PeerObjectImpl*>::const_iterator it =
           new_peer_objects.begin();
       it != new_peer_objects.end(); ++it) {
    SharedObject* const shared_object = it->first;
    PeerObjectImpl* const peer_object = it->second;

    shared_object->AddPeerObject(peer_object);
    CHECK_EQ(peer_object->SetSharedObjectIfUnset(shared_object), shared_object);
  }
}

SharedObject* TransactionStore::ConvertPeerObjectToSharedObject(
    PeerObjectImpl* peer_object) {
  if (peer_object == NULL) {
    return NULL;
  }

  SharedObject* shared_object = NULL;
  GetOrCreateSharedObjectForPeerObject(peer_object, &shared_object);

  return shared_object;
}

void TransactionStore::ConvertPendingEventToCommittedEvents(
    const PendingEvent* pending_event, const CanonicalPeer* origin_peer,
    unordered_map<SharedObject*, linked_ptr<SharedObjectTransactionInfo> >*
        shared_object_transactions) {
  CHECK(pending_event != NULL);

  const unordered_set<PeerObjectImpl*>& new_peer_objects =
      pending_event->new_peer_objects();
  unordered_set<SharedObject*> new_shared_objects;

  for (unordered_set<PeerObjectImpl*>::const_iterator it =
           new_peer_objects.begin();
       it != new_peer_objects.end(); ++it) {
    SharedObject* shared_object = NULL;
    GetOrCreateSharedObjectForPeerObject(*it, &shared_object);

    CHECK(new_shared_objects.insert(shared_object).second);
  }

  SharedObject* const prev_shared_object = ConvertPeerObjectToSharedObject(
      pending_event->prev_peer_object());

  CHECK(new_shared_objects.find(prev_shared_object) ==
            new_shared_objects.end());

  const unordered_map<PeerObjectImpl*, ConstLiveObjectPtr>& live_objects =
      pending_event->live_objects();

  for (unordered_map<PeerObjectImpl*, ConstLiveObjectPtr>::const_iterator it =
           live_objects.begin();
       it != live_objects.end(); ++it) {
    PeerObjectImpl* const peer_object = it->first;
    const ConstLiveObjectPtr& live_object = it->second;

    SharedObject* shared_object = NULL;
    GetOrCreateSharedObjectForPeerObject(peer_object, &shared_object);

    AddEventToSharedObjectTransactions(
        shared_object, origin_peer,
        new ObjectCreationCommittedEvent(live_object),
        shared_object_transactions);
  }

  const PendingEvent::Type type = pending_event->type();

  switch (type) {
    case PendingEvent::BEGIN_TRANSACTION:
      CHECK_EQ(new_shared_objects.size(), 0u);
      AddEventToSharedObjectTransactions(prev_shared_object, origin_peer,
                                         new BeginTransactionCommittedEvent(),
                                         shared_object_transactions);
      break;

    case PendingEvent::END_TRANSACTION:
      CHECK_EQ(new_shared_objects.size(), 0u);
      AddEventToSharedObjectTransactions(prev_shared_object, origin_peer,
                                         new EndTransactionCommittedEvent(),
                                         shared_object_transactions);
      break;

    case PendingEvent::METHOD_CALL: {
      PeerObjectImpl* next_peer_object = NULL;
      const string* method_name = NULL;
      const vector<Value>* parameters = NULL;

      pending_event->GetMethodCall(&next_peer_object, &method_name,
                                   &parameters);

      SharedObject* const next_shared_object = ConvertPeerObjectToSharedObject(
          next_peer_object);

      const vector<Value>::size_type parameter_count = parameters->size();
      vector<CommittedValue> committed_parameters(parameter_count);

      for (vector<Value>::size_type i = 0; i < parameter_count; ++i) {
        ConvertValueToCommittedValue((*parameters)[i],
                                     &committed_parameters[i]);
      }

      if (prev_shared_object == next_shared_object) {
        AddEventToSharedObjectTransactions(
            prev_shared_object, origin_peer,
            new SelfMethodCallCommittedEvent(new_shared_objects, *method_name,
                                             committed_parameters),
            shared_object_transactions);
      } else {
        if (prev_shared_object != NULL) {
          AddEventToSharedObjectTransactions(
              prev_shared_object, origin_peer,
              new SubMethodCallCommittedEvent(new_shared_objects,
                                              next_shared_object, *method_name,
                                              committed_parameters),
              shared_object_transactions);
        }

        AddEventToSharedObjectTransactions(
            next_shared_object, origin_peer,
            new MethodCallCommittedEvent(prev_shared_object, *method_name,
                                         committed_parameters),
            shared_object_transactions);
      }
      break;
    }

    case PendingEvent::METHOD_RETURN: {
      PeerObjectImpl* next_peer_object = NULL;
      const Value* return_value = NULL;

      pending_event->GetMethodReturn(&next_peer_object, &return_value);

      SharedObject* const next_shared_object = ConvertPeerObjectToSharedObject(
          next_peer_object);

      CommittedValue committed_return_value;
      ConvertValueToCommittedValue(*return_value, &committed_return_value);

      if (prev_shared_object == next_shared_object) {
        AddEventToSharedObjectTransactions(
            prev_shared_object, origin_peer,
            new SelfMethodReturnCommittedEvent(new_shared_objects,
                                               committed_return_value),
            shared_object_transactions);
      } else {
        AddEventToSharedObjectTransactions(
            prev_shared_object, origin_peer,
            new MethodReturnCommittedEvent(new_shared_objects,
                                           next_shared_object,
                                           committed_return_value),
            shared_object_transactions);

        if (next_shared_object != NULL) {
          AddEventToSharedObjectTransactions(
              next_shared_object, origin_peer,
              new SubMethodReturnCommittedEvent(prev_shared_object,
                                                committed_return_value),
              shared_object_transactions);
        }
      }
      break;
    }

    default:
      LOG(FATAL) << "Invalid pending event type: " << static_cast<int>(type);
  }
}

#define CONVERT_VALUE(enum_const, setter_method, getter_method) \
  case Value::enum_const: \
    out->setter_method(in.getter_method()); \
    break;

void TransactionStore::ConvertValueToCommittedValue(const Value& in,
                                                    CommittedValue* out) {
  CHECK(out != NULL);

  out->set_local_type(in.local_type());
  const Value::Type type = in.type();

  switch (type) {
    case Value::EMPTY:
      out->set_empty();
      break;

    CONVERT_VALUE(DOUBLE, set_double_value, double_value);
    CONVERT_VALUE(FLOAT, set_float_value, float_value);
    CONVERT_VALUE(INT64, set_int64_value, int64_value);
    CONVERT_VALUE(UINT64, set_uint64_value, uint64_value);
    CONVERT_VALUE(BOOL, set_bool_value, bool_value);
    CONVERT_VALUE(STRING, set_string_value, string_value);
    CONVERT_VALUE(BYTES, set_bytes_value, bytes_value);

    case Value::PEER_OBJECT: {
      PeerObjectImpl* const peer_object = static_cast<PeerObjectImpl*>(
          in.peer_object());
      out->set_shared_object(ConvertPeerObjectToSharedObject(peer_object));
      break;
    }

    default:
      LOG(FATAL) << "Unexpected value type: " << type;
  }
}

#undef CONVERT_VALUE

void TransactionStore::ConvertCommittedEventToEventProto(
    const CommittedEvent* in, EventProto* out) {
  CHECK(in != NULL);
  CHECK(out != NULL);

  const CommittedEvent::Type type = in->type();

  switch (type) {
    case CommittedEvent::OBJECT_CREATION: {
      ConstLiveObjectPtr live_object;
      in->GetObjectCreation(&live_object);

      ObjectCreationEventProto* const object_creation_event_proto =
          out->mutable_object_creation();

      vector<PeerObjectImpl*> referenced_peer_objects;
      live_object->Serialize(object_creation_event_proto->mutable_data(),
                             &referenced_peer_objects);

      for (vector<PeerObjectImpl*>::const_iterator it =
               referenced_peer_objects.begin();
           it != referenced_peer_objects.end(); ++it) {
        PeerObjectImpl* const peer_object = *it;
        SharedObject* shared_object = NULL;
        GetOrCreateSharedObjectForPeerObject(peer_object, &shared_object);

        object_creation_event_proto->add_referenced_object_id()->CopyFrom(
            shared_object->object_id());
      }
      break;
    }

    case CommittedEvent::BEGIN_TRANSACTION:
      out->mutable_begin_transaction();
      break;

    case CommittedEvent::END_TRANSACTION:
      out->mutable_end_transaction();
      break;

    case CommittedEvent::METHOD_CALL: {
      SharedObject* caller = NULL;
      const string* method_name = NULL;
      const vector<CommittedValue>* parameters = NULL;

      in->GetMethodCall(&caller, &method_name, &parameters);

      MethodCallEventProto* const method_call_event_proto =
          out->mutable_method_call();
      method_call_event_proto->set_method_name(*method_name);

      for (vector<CommittedValue>::const_iterator it = parameters->begin();
           it != parameters->end(); ++it) {
        ConvertCommittedValueToValueProto(
            *it, method_call_event_proto->add_parameter());
      }

      if (caller != NULL) {
        method_call_event_proto->mutable_caller_object_id()->CopyFrom(
            caller->object_id());
      }

      break;
    }

    case CommittedEvent::METHOD_RETURN: {
      SharedObject* caller = NULL;
      const CommittedValue* return_value = NULL;

      in->GetMethodReturn(&caller, &return_value);

      MethodReturnEventProto* const method_return_event_proto =
          out->mutable_method_return();
      ConvertCommittedValueToValueProto(
          *return_value, method_return_event_proto->mutable_return_value());

      if (caller != NULL) {
        method_return_event_proto->mutable_caller_object_id()->CopyFrom(
            caller->object_id());
      }

      break;
    }

    case CommittedEvent::SUB_METHOD_CALL: {
      SharedObject* callee = NULL;
      const string* method_name = NULL;
      const vector<CommittedValue>* parameters = NULL;

      in->GetSubMethodCall(&callee, &method_name, &parameters);

      SubMethodCallEventProto* const sub_method_call_event_proto =
          out->mutable_sub_method_call();
      sub_method_call_event_proto->set_method_name(*method_name);

      for (vector<CommittedValue>::const_iterator it = parameters->begin();
           it != parameters->end(); ++it) {
        ConvertCommittedValueToValueProto(
            *it, sub_method_call_event_proto->add_parameter());
      }

      sub_method_call_event_proto->mutable_callee_object_id()->CopyFrom(
          callee->object_id());

      break;
    }

    case CommittedEvent::SUB_METHOD_RETURN: {
      SharedObject* callee = NULL;
      const CommittedValue* return_value = NULL;

      in->GetSubMethodReturn(&callee, &return_value);

      SubMethodReturnEventProto* const sub_method_return_event_proto =
          out->mutable_sub_method_return();
      ConvertCommittedValueToValueProto(
          *return_value, sub_method_return_event_proto->mutable_return_value());

      sub_method_return_event_proto->mutable_callee_object_id()->CopyFrom(
          callee->object_id());

      break;
    }

    case CommittedEvent::SELF_METHOD_CALL: {
      const string* method_name = NULL;
      const vector<CommittedValue>* parameters = NULL;

      in->GetSelfMethodCall(&method_name, &parameters);

      SelfMethodCallEventProto* const self_method_call_event_proto =
          out->mutable_self_method_call();
      self_method_call_event_proto->set_method_name(*method_name);

      for (vector<CommittedValue>::const_iterator it = parameters->begin();
           it != parameters->end(); ++it) {
        ConvertCommittedValueToValueProto(
            *it, self_method_call_event_proto->add_parameter());
      }

      break;
    }

    case CommittedEvent::SELF_METHOD_RETURN: {
      const CommittedValue* return_value = NULL;

      in->GetSelfMethodReturn(&return_value);

      SelfMethodReturnEventProto* const self_method_return_event_proto =
          out->mutable_self_method_return();
      ConvertCommittedValueToValueProto(
          *return_value,
          self_method_return_event_proto->mutable_return_value());

      break;
    }

    default:
      LOG(FATAL) << "Invalid committed event type: " << static_cast<int>(type);
  }

  const unordered_set<SharedObject*>& new_shared_objects =
      in->new_shared_objects();

  for (unordered_set<SharedObject*>::const_iterator it =
           new_shared_objects.begin();
       it != new_shared_objects.end(); ++it) {
    out->add_new_object_id()->CopyFrom((*it)->object_id());
  }
}

CommittedEvent* TransactionStore::ConvertEventProtoToCommittedEvent(
    const EventProto& event_proto) {
  unordered_set<SharedObject*> new_shared_objects;
  for (int i = 0; i < event_proto.new_object_id_size(); ++i) {
    new_shared_objects.insert(
        GetOrCreateSharedObject(event_proto.new_object_id(i)));
  }

  const EventProto::Type type = GetEventProtoType(event_proto);

  switch (type) {
    case EventProto::OBJECT_CREATION: {
      CHECK_EQ(new_shared_objects.size(), 0u);

      const ObjectCreationEventProto& object_creation_event_proto =
          event_proto.object_creation();

      const int referenced_object_count =
          object_creation_event_proto.referenced_object_id_size();
      vector<PeerObjectImpl*> referenced_peer_objects(referenced_object_count);

      for (int i = 0; i < referenced_object_count; ++i) {
        const Uuid& object_id =
            object_creation_event_proto.referenced_object_id(i);
        SharedObject* const referenced_shared_object = GetOrCreateSharedObject(
            object_id);
        referenced_peer_objects[i] =
            referenced_shared_object->GetOrCreatePeerObject();
      }

      const ConstLiveObjectPtr live_object(
          new LiveObject(
              DeserializeLocalObjectFromString(
                  interpreter_, object_creation_event_proto.data(),
                  referenced_peer_objects)));

      return new ObjectCreationCommittedEvent(live_object);
    }

    case EventProto::BEGIN_TRANSACTION:
      CHECK_EQ(new_shared_objects.size(), 0u);
      return new BeginTransactionCommittedEvent();

    case EventProto::END_TRANSACTION:
      CHECK_EQ(new_shared_objects.size(), 0u);
      return new EndTransactionCommittedEvent();

    case EventProto::METHOD_CALL: {
      CHECK_EQ(new_shared_objects.size(), 0u);

      const MethodCallEventProto& method_call_event_proto =
          event_proto.method_call();

      SharedObject* caller = NULL;
      if (method_call_event_proto.has_caller_object_id()) {
        caller = GetOrCreateSharedObject(
            method_call_event_proto.caller_object_id());
      }

      const string& method_name = method_call_event_proto.method_name();

      const int parameter_count = method_call_event_proto.parameter_size();
      vector<CommittedValue> parameters(parameter_count);

      for (int i = 0; i < parameter_count; ++i) {
        ConvertValueProtoToCommittedValue(method_call_event_proto.parameter(i),
                                          &parameters[i]);
      }

      return new MethodCallCommittedEvent(caller, method_name, parameters);
    }

    case EventProto::METHOD_RETURN: {
      const MethodReturnEventProto& method_return_event_proto =
          event_proto.method_return();

      SharedObject* caller = NULL;
      if (method_return_event_proto.has_caller_object_id()) {
        caller = GetOrCreateSharedObject(
            method_return_event_proto.caller_object_id());
      }

      CommittedValue return_value;
      ConvertValueProtoToCommittedValue(
          method_return_event_proto.return_value(), &return_value);

      return new MethodReturnCommittedEvent(new_shared_objects, caller,
                                            return_value);
    }

    case EventProto::SUB_METHOD_CALL: {
      const SubMethodCallEventProto& sub_method_call_event_proto =
          event_proto.sub_method_call();

      SharedObject* const callee = GetOrCreateSharedObject(
          sub_method_call_event_proto.callee_object_id());
      const string& method_name = sub_method_call_event_proto.method_name();

      const int parameter_count = sub_method_call_event_proto.parameter_size();
      vector<CommittedValue> parameters(parameter_count);

      for (int i = 0; i < parameter_count; ++i) {
        ConvertValueProtoToCommittedValue(
            sub_method_call_event_proto.parameter(i), &parameters[i]);
      }

      return new SubMethodCallCommittedEvent(new_shared_objects, callee,
                                             method_name, parameters);
    }

    case EventProto::SUB_METHOD_RETURN: {
      CHECK_EQ(new_shared_objects.size(), 0u);

      const SubMethodReturnEventProto& sub_method_return_event_proto =
          event_proto.sub_method_return();

      SharedObject* const callee = GetOrCreateSharedObject(
          sub_method_return_event_proto.callee_object_id());

      CommittedValue return_value;
      ConvertValueProtoToCommittedValue(
          sub_method_return_event_proto.return_value(), &return_value);

      return new SubMethodReturnCommittedEvent(callee, return_value);
    }

    case EventProto::SELF_METHOD_CALL: {
      const SelfMethodCallEventProto& self_method_call_event_proto =
          event_proto.self_method_call();

      const string& method_name = self_method_call_event_proto.method_name();

      const int parameter_count = self_method_call_event_proto.parameter_size();
      vector<CommittedValue> parameters(parameter_count);

      for (int i = 0; i < parameter_count; ++i) {
        ConvertValueProtoToCommittedValue(
            self_method_call_event_proto.parameter(i), &parameters[i]);
      }

      return new SelfMethodCallCommittedEvent(new_shared_objects, method_name,
                                              parameters);
    }

    case EventProto::SELF_METHOD_RETURN: {
      const SelfMethodReturnEventProto& self_method_return_event_proto =
          event_proto.self_method_return();

      CommittedValue return_value;
      ConvertValueProtoToCommittedValue(
          self_method_return_event_proto.return_value(), &return_value);

      return new SelfMethodReturnCommittedEvent(new_shared_objects,
                                                return_value);
    }

    default:
      LOG(FATAL) << "Invalid event type: " << static_cast<int>(type);
  }

  return NULL;
}

#define CONVERT_VALUE(enum_const, setter_method, getter_method) \
  case ValueProto::enum_const: \
    out->setter_method(in.getter_method()); \
    break;

void TransactionStore::ConvertValueProtoToCommittedValue(const ValueProto& in,
                                                         CommittedValue* out) {
  CHECK(out != NULL);

  out->set_local_type(in.local_type());
  const ValueProto::Type type = GetValueProtoType(in);

  switch (type) {
    case ValueProto::EMPTY:
      out->set_empty();
      break;

    CONVERT_VALUE(DOUBLE, set_double_value, double_value);
    CONVERT_VALUE(FLOAT, set_float_value, float_value);
    CONVERT_VALUE(INT64, set_int64_value, int64_value);
    CONVERT_VALUE(UINT64, set_uint64_value, uint64_value);
    CONVERT_VALUE(BOOL, set_bool_value, bool_value);
    CONVERT_VALUE(STRING, set_string_value, string_value);
    CONVERT_VALUE(BYTES, set_bytes_value, bytes_value);

    case ValueProto::OBJECT_ID: {
      out->set_shared_object(GetOrCreateSharedObject(in.object_id()));
      break;
    }

    default:
      LOG(FATAL) << "Unexpected value proto type: " << static_cast<int>(type);
  }
}

#undef CONVERT_VALUE

// static
void TransactionStore::AddEventToSharedObjectTransactions(
    SharedObject* shared_object,
    const CanonicalPeer* origin_peer,
    CommittedEvent* event,
    unordered_map<SharedObject*, linked_ptr<SharedObjectTransactionInfo> >*
        shared_object_transactions) {
  CHECK(shared_object != NULL);
  CHECK(origin_peer != NULL);
  CHECK(event != NULL);
  CHECK(shared_object_transactions != NULL);

  linked_ptr<SharedObjectTransactionInfo>& transaction =
      (*shared_object_transactions)[shared_object];
  if (transaction.get() == NULL) {
    transaction.reset(new SharedObjectTransactionInfo());
    transaction->origin_peer = origin_peer;
  } else {
    CHECK_EQ(transaction->origin_peer, origin_peer);
  }

  transaction->events.push_back(make_linked_ptr(event));
}

}  // namespace peer
}  // namespace floating_temple