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

#ifndef PEER_INTERPRETER_THREAD_H_
#define PEER_INTERPRETER_THREAD_H_

#include <pthread.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "base/cond_var.h"
#include "base/linked_ptr.h"
#include "base/macros.h"
#include "base/mutex.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "peer/proto/transaction_id.pb.h"

namespace floating_temple {

class VersionedLocalObject;

namespace peer {

class LiveObject;
class PeerObjectImpl;
class PendingEvent;
class SequencePoint;
class TransactionStoreInternalInterface;

// TODO(dss): Rename this class to RecordingThread.
// TODO(dss): Make this class inherit privately from class Thread.
class InterpreterThread : public Thread {
 public:
  explicit InterpreterThread(
      TransactionStoreInternalInterface* transaction_store);
  ~InterpreterThread() override;

  void RunProgram(VersionedLocalObject* local_object,
                  const std::string& method_name,
                  Value* return_value,
                  bool linger);

  void Rewind(const TransactionId& rejected_transaction_id);
  void Resume();

  bool BeginTransaction() override;
  bool EndTransaction() override;
  PeerObject* CreatePeerObject(VersionedLocalObject* initial_version,
                               const std::string& name,
                               bool versioned) override;
  bool CallMethod(PeerObject* peer_object,
                  const std::string& method_name,
                  const std::vector<Value>& parameters,
                  Value* return_value) override;
  bool ObjectsAreEquivalent(const PeerObject* a,
                            const PeerObject* b) const override;

 private:
  struct NewObject {
    std::shared_ptr<const LiveObject> live_object;
    bool object_is_named;
  };

  bool CallMethodHelper(const TransactionId& method_call_transaction_id,
                        PeerObjectImpl* caller_peer_object,
                        PeerObjectImpl* callee_peer_object,
                        const std::string& method_name,
                        const std::vector<Value>& parameters,
                        std::shared_ptr<LiveObject>* callee_live_object,
                        Value* return_value);
  bool WaitForBlockingThreads_Locked(
      const TransactionId& method_call_transaction_id) const;

  std::shared_ptr<LiveObject> GetLiveObject(PeerObjectImpl* peer_object);
  const SequencePoint* GetSequencePoint();

  void AddTransactionEvent(PendingEvent* event);
  void CommitTransaction();

  void CheckIfValueIsNew(
      const Value& value,
      std::unordered_map<PeerObjectImpl*, std::shared_ptr<const LiveObject>>*
          live_objects,
      std::unordered_set<PeerObjectImpl*>* new_peer_objects);
  void CheckIfPeerObjectIsNew(
      PeerObjectImpl* peer_object,
      std::unordered_map<PeerObjectImpl*, std::shared_ptr<const LiveObject>>*
          live_objects,
      std::unordered_set<PeerObjectImpl*>* new_peer_objects);

  bool Rewinding() const;
  bool Rewinding_Locked() const;

  TransactionStoreInternalInterface* const transaction_store_;

  int transaction_level_;
  std::vector<linked_ptr<PendingEvent>> events_;
  std::unordered_map<PeerObjectImpl*, NewObject> new_objects_;
  std::unordered_map<PeerObjectImpl*, std::shared_ptr<LiveObject>>
      modified_objects_;
  std::unique_ptr<SequencePoint> sequence_point_;
  bool committing_transaction_;

  PeerObjectImpl* current_peer_object_;
  std::shared_ptr<LiveObject> current_live_object_;

  // ID of the last transaction that was committed by this thread.
  TransactionId current_transaction_id_;

  // If rejected_transaction_id_ is valid, then all transactions starting with
  // (and including) that transaction ID have been rejected. This thread should
  // rewind past the start of the first rejected transaction, clear
  // rejected_transaction_id_, and then resume execution.
  //
  // To clear rejected_transaction_id_, set it to the minimum transaction ID by
  // calling GetMinTransactionId.
  TransactionId rejected_transaction_id_;
  std::unordered_set<pthread_t> blocking_threads_;
  mutable CondVar rewinding_cond_;
  mutable CondVar blocking_threads_empty_cond_;
  mutable Mutex rejected_transaction_id_mu_;

  DISALLOW_COPY_AND_ASSIGN(InterpreterThread);
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_INTERPRETER_THREAD_H_
