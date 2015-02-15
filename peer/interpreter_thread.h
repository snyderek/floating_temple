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

#include <string>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include <vector>

#include "base/cond_var.h"
#include "base/linked_ptr.h"
#include "base/macros.h"
#include "base/mutex.h"
#include "base/scoped_ptr.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "peer/const_live_object_ptr.h"
#include "peer/live_object_ptr.h"
#include "peer/proto/transaction_id.pb.h"

namespace floating_temple {
namespace peer {

class PeerObjectImpl;
class PendingEvent;
class SequencePoint;
class TransactionStoreInternalInterface;

class InterpreterThread : public Thread {
 public:
  explicit InterpreterThread(
      TransactionStoreInternalInterface* transaction_store);
  virtual ~InterpreterThread();

  void Rewind(const TransactionId& rejected_transaction_id);
  void Resume();

  virtual bool BeginTransaction();
  virtual bool EndTransaction();
  virtual PeerObject* CreatePeerObject(LocalObject* initial_version);
  virtual PeerObject* GetOrCreateNamedObject(const std::string& name,
                                             LocalObject* initial_version);
  virtual bool CallMethod(PeerObject* peer_object,
                          const std::string& method_name,
                          const std::vector<Value>& parameters,
                          Value* return_value);
  virtual bool ObjectsAreEquivalent(const PeerObject* a,
                                    const PeerObject* b) const;

 private:
  struct NewObject {
    ConstLiveObjectPtr live_object;
    bool object_is_named;
  };

  bool CallMethodHelper(const TransactionId& method_call_transaction_id,
                        PeerObjectImpl* caller_peer_object,
                        PeerObjectImpl* callee_peer_object,
                        const std::string& method_name,
                        const std::vector<Value>& parameters,
                        LiveObjectPtr* callee_live_object,
                        Value* return_value);
  bool WaitForBlockingThreads_Locked(
      const TransactionId& method_call_transaction_id) const;

  LiveObjectPtr GetLiveObject(PeerObjectImpl* peer_object);
  const SequencePoint* GetSequencePoint();

  void AddTransactionEvent(PendingEvent* event);
  void CommitTransaction();

  void CheckIfValueIsNew(
      const Value& value,
      std::tr1::unordered_map<PeerObjectImpl*, ConstLiveObjectPtr>*
          live_objects,
      std::tr1::unordered_set<PeerObjectImpl*>* new_peer_objects);
  void CheckIfPeerObjectIsNew(
      PeerObjectImpl* peer_object,
      std::tr1::unordered_map<PeerObjectImpl*, ConstLiveObjectPtr>*
          live_objects,
      std::tr1::unordered_set<PeerObjectImpl*>* new_peer_objects);

  bool Rewinding() const;
  bool Rewinding_Locked() const;

  TransactionStoreInternalInterface* const transaction_store_;

  int transaction_level_;
  std::vector<linked_ptr<PendingEvent> > events_;
  std::tr1::unordered_map<PeerObjectImpl*, NewObject> new_objects_;
  std::tr1::unordered_map<PeerObjectImpl*, LiveObjectPtr> modified_objects_;
  scoped_ptr<SequencePoint> sequence_point_;

  PeerObjectImpl* current_peer_object_;
  LiveObjectPtr current_live_object_;

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
  std::tr1::unordered_set<pthread_t> blocking_threads_;
  mutable CondVar blocking_threads_empty_cond_;
  mutable Mutex rejected_transaction_id_mu_;

  DISALLOW_COPY_AND_ASSIGN(InterpreterThread);
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_INTERPRETER_THREAD_H_
