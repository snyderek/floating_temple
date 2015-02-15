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

#ifndef PEER_PEER_THREAD_H_
#define PEER_PEER_THREAD_H_

#include <pthread.h>

#include <string>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include <vector>

#include "base/macros.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "peer/committed_event.h"
#include "peer/committed_value.h"
#include "peer/const_live_object_ptr.h"
#include "peer/event_queue.h"
#include "peer/live_object_ptr.h"
#include "util/bool_variable.h"
#include "util/state_variable.h"

namespace floating_temple {

class StateVariableInternalInterface;

namespace peer {

class CommittedEvent;
class PeerObjectImpl;
class SharedObject;
class TransactionStoreInternalInterface;

class PeerThread : private Thread {
 public:
  PeerThread();
  virtual ~PeerThread();

  ConstLiveObjectPtr live_object() const;

  // Be sure to call FlushEvents() or Stop() before calling this method.
  bool conflict_detected() const { return conflict_detected_.Get(); }

  void Start(TransactionStoreInternalInterface* transaction_store,
             SharedObject* shared_object,
             const LiveObjectPtr& live_object,
             std::tr1::unordered_map<SharedObject*, PeerObjectImpl*>*
                 new_peer_objects);
  void Stop();

  void QueueEvent(const CommittedEvent* event);
  void FlushEvents();

 private:
  enum {
    NOT_STARTED = 0x1,
    STARTING = 0x2,
    RUNNING = 0x4,
    PAUSED = 0x8,
    STOPPING = 0x10,
    STOPPED = 0x20
  };

  void ReplayEvents();

  // TODO(dss): Rename these methods.
  void DoMethodCall();
  void DoSelfMethodCall(PeerObjectImpl* peer_object,
                        const std::string& method_name,
                        const std::vector<Value>& parameters,
                        Value* return_value);
  void DoSubMethodCall(PeerObjectImpl* peer_object,
                       const std::string& method_name,
                       const std::vector<Value>& parameters,
                       Value* return_value);

  bool HasNextEvent();
  CommittedEvent::Type PeekNextEventType();
  const CommittedEvent* GetNextEvent();
  bool CheckNextEventType(CommittedEvent::Type actual_event_type);

  bool MethodCallMatches(
      SharedObject* expected_shared_object,
      const std::string& expected_method_name,
      const std::vector<CommittedValue>& expected_parameters,
      PeerObjectImpl* peer_object,
      const std::string& method_name,
      const std::vector<Value>& parameters,
      const std::tr1::unordered_set<SharedObject*>& new_shared_objects);
  bool ValueMatches(
      const CommittedValue& committed_value, const Value& pending_value,
      const std::tr1::unordered_set<SharedObject*>& new_shared_objects);
  bool ObjectMatches(
      SharedObject* shared_object, PeerObjectImpl* peer_object,
      const std::tr1::unordered_set<SharedObject*>& new_shared_objects);

  void SetConflictDetected(const std::string& description);

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

  static void* ReplayThreadMain(void* peer_thread_raw);

  static void ChangeRunningToPaused(
      StateVariableInternalInterface* state_variable);
  static void ChangePausedToRunning(
      StateVariableInternalInterface* state_variable);
  static void ChangeToPausedAndWaitForRunning(
      StateVariableInternalInterface* state_variable);
  static void WaitForPausedAndChangeToStopping(
      StateVariableInternalInterface* state_variable);

  TransactionStoreInternalInterface* transaction_store_;
  SharedObject* shared_object_;
  LiveObjectPtr live_object_;
  std::tr1::unordered_map<SharedObject*, PeerObjectImpl*>* new_peer_objects_;

  pthread_t replay_thread_;
  EventQueue event_queue_;
  std::tr1::unordered_set<PeerObjectImpl*> unbound_peer_objects_;

  BoolVariable conflict_detected_;

  StateVariable state_;

  DISALLOW_COPY_AND_ASSIGN(PeerThread);
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_PEER_THREAD_H_