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

#ifndef PEER_TRANSACTION_STORE_H_
#define PEER_TRANSACTION_STORE_H_

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/cond_var.h"
#include "base/integral_types.h"
#include "base/linked_ptr.h"
#include "base/macros.h"
#include "base/mutex.h"
#include "peer/connection_handler.h"
#include "peer/proto/transaction_id.pb.h"
#include "peer/proto/uuid.pb.h"
#include "peer/sequence_point_impl.h"
#include "peer/transaction_id_generator.h"
#include "peer/transaction_id_util.h"
#include "peer/transaction_sequencer.h"
#include "peer/transaction_store_internal_interface.h"

namespace floating_temple {

class Interpreter;
class Value;

namespace peer {

class ApplyTransactionMessage;
class CanonicalPeer;
class CanonicalPeerMap;
class CommittedEvent;
class CommittedValue;
class EventProto;
class GetObjectMessage;
class InvalidateTransactionsMessage;
class PeerMessage;
class PeerMessageSender;
class PeerObjectImpl;
class PendingEvent;
class RecordingThread;
class RejectTransactionMessage;
class SharedObject;
class SharedObjectTransaction;
class StoreObjectMessage;
class ValueProto;

class TransactionStore : public ConnectionHandler,
                         private TransactionStoreInternalInterface {
 public:
  TransactionStore(CanonicalPeerMap* canonical_peer_map,
                   PeerMessageSender* peer_message_sender,
                   Interpreter* interpreter,
                   const CanonicalPeer* local_peer,
                   bool delay_object_binding);
  ~TransactionStore() override;

  // The caller must not take ownership of the returned RecordingThread
  // instance.
  RecordingThread* CreateRecordingThread();

  void NotifyNewConnection(const CanonicalPeer* remote_peer) override;
  // TODO(dss): Move parsing of the peer message to the ConnectionManager class.
  void HandleMessageFromRemotePeer(const CanonicalPeer* remote_peer,
                                   const PeerMessage& peer_message) override;

 private:
  static const char kObjectNamespaceUuidString[];

  struct UuidHasher {
    std::size_t operator()(const Uuid& uuid) const;
  };
  struct UuidEquals {
    bool operator()(const Uuid& a, const Uuid& b) const;
  };

  typedef std::unordered_map<Uuid, linked_ptr<SharedObject>,
                             UuidHasher, UuidEquals> SharedObjectMap;

  bool delay_object_binding() const override { return delay_object_binding_; }
  SequencePoint* GetCurrentSequencePoint() const override;
  std::shared_ptr<const LiveObject> GetLiveObjectAtSequencePoint(
      PeerObjectImpl* peer_object, const SequencePoint* sequence_point,
      bool wait) override;
  PeerObjectImpl* CreateUnboundPeerObject(bool versioned) override;
  PeerObjectImpl* CreateBoundPeerObject(const std::string& name,
                                        bool versioned) override;
  void CreateTransaction(
      const std::vector<linked_ptr<PendingEvent>>& events,
      TransactionId* transaction_id,
      const std::unordered_map<PeerObjectImpl*, std::shared_ptr<LiveObject>>&
          modified_objects,
      const SequencePoint* prev_sequence_point) override;
  bool ObjectsAreEquivalent(const PeerObjectImpl* a,
                            const PeerObjectImpl* b) const override;

  void HandleApplyTransactionMessage(
      const CanonicalPeer* remote_peer,
      const ApplyTransactionMessage& apply_transaction_message);
  void HandleGetObjectMessage(
      const CanonicalPeer* remote_peer,
      const GetObjectMessage& get_object_message);
  void HandleStoreObjectMessage(
      const CanonicalPeer* remote_peer,
      const StoreObjectMessage& store_object_message);
  void HandleRejectTransactionMessage(
      const CanonicalPeer* remote_peer,
      const RejectTransactionMessage& reject_transaction_message);
  void HandleInvalidateTransactionsMessage(
      const CanonicalPeer* remote_peer,
      const InvalidateTransactionsMessage& invalidate_transactions_message);

  SharedObject* GetSharedObject(const Uuid& object_id) const;
  SharedObject* GetOrCreateSharedObject(const Uuid& object_id);

  std::shared_ptr<const LiveObject> GetLiveObjectAtSequencePoint_Helper(
      SharedObject* shared_object,
      const SequencePointImpl& sequence_point_impl,
      uint64* current_version_number,
      std::unordered_map<SharedObject*, PeerObjectImpl*>* new_peer_objects,
      std::vector<std::pair<const CanonicalPeer*, TransactionId>>*
          all_transactions_to_reject);

  // TODO(dss): Change the semantics of these methods so that they don't modify
  // the map that 'shared_object_transactions' points to. This will make the
  // methods' interfaces more intuitive.
  void ApplyTransactionAndSendMessage(
      const TransactionId& transaction_id,
      std::unordered_map<SharedObject*, linked_ptr<SharedObjectTransaction>>*
          shared_object_transactions);
  void ApplyTransaction(
      const TransactionId& transaction_id,
      const CanonicalPeer* origin_peer,
      std::unordered_map<SharedObject*, linked_ptr<SharedObjectTransaction>>*
          shared_object_transactions);

  void RejectTransactionsAndSendMessages(
      const std::vector<std::pair<const CanonicalPeer*, TransactionId>>&
          transactions_to_reject,
      const TransactionId& new_transaction_id);
  void RejectTransactions(
      const std::vector<std::pair<const CanonicalPeer*, TransactionId>>&
          transactions_to_reject,
      const TransactionId& new_transaction_id,
      RejectTransactionMessage* reject_transaction_message);

  void SendMessageToAffectedPeers(
      const PeerMessage& peer_message,
      const std::unordered_set<SharedObject*>& affected_objects);

  void UpdateCurrentSequencePoint(const CanonicalPeer* origin_peer,
                                  const TransactionId& transaction_id);

  // current_sequence_point_mu_ must be locked.
  void IncrementVersionNumber_Locked();

  void CreateNewPeerObjects(
      const std::unordered_map<SharedObject*, PeerObjectImpl*>&
          new_peer_objects);

  SharedObject* GetSharedObjectForPeerObject(PeerObjectImpl* peer_object);

  void ConvertPendingEventToCommittedEvents(
      const PendingEvent* pending_event, const CanonicalPeer* origin_peer,
      std::unordered_map<SharedObject*, linked_ptr<SharedObjectTransaction>>*
          shared_object_transactions);
  void ConvertValueToCommittedValue(const Value& in, CommittedValue* out);

  void ConvertCommittedEventToEventProto(const CommittedEvent* in,
                                         EventProto* out);

  CommittedEvent* ConvertEventProtoToCommittedEvent(
      const EventProto& event_proto);
  void ConvertValueProtoToCommittedValue(const ValueProto& in,
                                         CommittedValue* out);

  static void AddEventToSharedObjectTransactions(
      SharedObject* shared_object,
      const CanonicalPeer* origin_peer,
      CommittedEvent* event,
      std::unordered_map<SharedObject*, linked_ptr<SharedObjectTransaction>>*
          shared_object_transactions);

  CanonicalPeerMap* const canonical_peer_map_;
  Interpreter* const interpreter_;
  const CanonicalPeer* const local_peer_;
  const bool delay_object_binding_;
  const Uuid object_namespace_uuid_;

  TransactionIdGenerator transaction_id_generator_;
  TransactionSequencer transaction_sequencer_;

  std::vector<linked_ptr<RecordingThread>> recording_threads_;
  mutable Mutex recording_threads_mu_;

  SharedObjectMap shared_objects_;
  mutable Mutex shared_objects_mu_;

  std::unordered_set<SharedObject*> named_objects_;
  mutable Mutex named_objects_mu_;

  std::vector<linked_ptr<PeerObjectImpl>> peer_objects_;
  mutable Mutex peer_objects_mu_;

  SequencePointImpl current_sequence_point_;
  uint64 version_number_;
  mutable CondVar version_number_changed_cond_;
  mutable Mutex current_sequence_point_mu_;

  DISALLOW_COPY_AND_ASSIGN(TransactionStore);
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_TRANSACTION_STORE_H_
