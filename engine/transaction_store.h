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

#ifndef ENGINE_TRANSACTION_STORE_H_
#define ENGINE_TRANSACTION_STORE_H_

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/cond_var.h"
#include "base/integral_types.h"
#include "base/macros.h"
#include "base/mutex.h"
#include "engine/connection_handler.h"
#include "engine/proto/transaction_id.pb.h"
#include "engine/proto/uuid.pb.h"
#include "engine/sequence_point_impl.h"
#include "engine/transaction_id_generator.h"
#include "engine/transaction_id_util.h"
#include "engine/transaction_sequencer.h"
#include "engine/transaction_store_internal_interface.h"

namespace floating_temple {

class Interpreter;
class LocalObject;
class Value;

namespace engine {

class ApplyTransactionMessage;
class CanonicalPeer;
class CanonicalPeerMap;
class CommittedEvent;
class EventProto;
class GetObjectMessage;
class InvalidateTransactionsMessage;
class ObjectReferenceImpl;
class PeerMessage;
class PeerMessageSender;
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
                   const CanonicalPeer* local_peer);
  ~TransactionStore() override;

  void RunProgram(LocalObject* local_object,
                  const std::string& method_name,
                  Value* return_value,
                  bool linger);

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

  typedef std::unordered_map<Uuid, std::unique_ptr<SharedObject>,
                             UuidHasher, UuidEquals> SharedObjectMap;

  const CanonicalPeer* GetLocalPeer() const override;
  SequencePoint* GetCurrentSequencePoint() const override;
  std::shared_ptr<const LiveObject> GetLiveObjectAtSequencePoint(
      ObjectReferenceImpl* object_reference,
      const SequencePoint* sequence_point, bool wait) override;
  ObjectReferenceImpl* CreateObjectReference(const std::string& name) override;
  void CreateTransaction(
      const std::unordered_map<ObjectReferenceImpl*,
                               std::unique_ptr<SharedObjectTransaction>>&
          object_transactions,
      TransactionId* transaction_id,
      const std::unordered_map<ObjectReferenceImpl*,
                               std::shared_ptr<LiveObject>>& modified_objects,
      const SequencePoint* prev_sequence_point) override;
  bool ObjectsAreIdentical(const ObjectReferenceImpl* a,
                           const ObjectReferenceImpl* b) const override;
  ExecutionPhase GetExecutionPhase(
      const TransactionId& base_transaction_id) override;
  void WaitForRewind() override;

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
      std::vector<std::pair<const CanonicalPeer*, TransactionId>>*
          all_transactions_to_reject);

  void ApplyTransactionAndSendMessage(
      const TransactionId& transaction_id,
      const std::unordered_map<SharedObject*,
                               std::unique_ptr<SharedObjectTransaction>>&
          shared_object_transactions);
  void ApplyTransaction(
      const TransactionId& transaction_id,
      const CanonicalPeer* origin_peer,
      const std::unordered_map<SharedObject*,
                               std::unique_ptr<SharedObjectTransaction>>&
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

  void ConvertCommittedEventToEventProto(const CommittedEvent* in,
                                         EventProto* out);

  CommittedEvent* ConvertEventProtoToCommittedEvent(
      const EventProto& event_proto);
  void ConvertValueProtoToValue(const ValueProto& in, Value* out);

  CanonicalPeerMap* const canonical_peer_map_;
  Interpreter* const interpreter_;
  const CanonicalPeer* const local_peer_;
  const Uuid object_namespace_uuid_;

  TransactionIdGenerator transaction_id_generator_;
  TransactionSequencer transaction_sequencer_;

  RecordingThread* recording_thread_;
  mutable Mutex recording_thread_mu_;

  // If rejected_transaction_id_ != MIN_TRANSACTION_ID, then all transactions
  // starting with (and including) that transaction ID have been rejected. The
  // recording thread should rewind past the start of the first rejected
  // transaction and then resume execution.
  //
  // To clear rejected_transaction_id_, set it equal to MIN_TRANSACTION_ID
  // (declared in "engine/transaction_id_util.h").
  TransactionId rejected_transaction_id_;
  mutable CondVar rewinding_cond_;
  mutable Mutex rejected_transaction_id_mu_;

  SharedObjectMap shared_objects_;
  mutable Mutex shared_objects_mu_;

  std::unordered_set<SharedObject*> named_objects_;
  mutable Mutex named_objects_mu_;

  SequencePointImpl current_sequence_point_;
  uint64 version_number_;
  mutable CondVar version_number_changed_cond_;
  mutable Mutex current_sequence_point_mu_;

  DISALLOW_COPY_AND_ASSIGN(TransactionStore);
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_TRANSACTION_STORE_H_
