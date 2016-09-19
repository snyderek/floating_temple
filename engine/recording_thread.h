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

#ifndef ENGINE_RECORDING_THREAD_H_
#define ENGINE_RECORDING_THREAD_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/macros.h"
#include "engine/recording_thread_internal_interface.h"
#include "include/c++/value.h"

namespace floating_temple {

class LocalObject;

namespace engine {

class CommittedEvent;
class LiveObject;
class ObjectReferenceImpl;
class PendingTransaction;
class TransactionStoreInternalInterface;

class RecordingThread : private RecordingThreadInternalInterface {
 public:
  explicit RecordingThread(
      TransactionStoreInternalInterface* transaction_store);
  ~RecordingThread() override;

  void RunProgram(LocalObject* local_object,
                  const std::string& method_name,
                  Value* return_value,
                  bool linger);

 private:
  bool BeginTransaction(
      ObjectReferenceImpl* caller_object_reference,
      const std::shared_ptr<LiveObject>& caller_live_object) override;
  bool EndTransaction(
      ObjectReferenceImpl* caller_object_reference,
      const std::shared_ptr<LiveObject>& caller_live_object) override;
  ObjectReferenceImpl* CreateObject(
      ObjectReferenceImpl* caller_object_reference,
      const std::shared_ptr<LiveObject>& caller_live_object,
      LocalObject* initial_version,
      const std::string& name) override;
  bool CallMethod(ObjectReferenceImpl* caller_object_reference,
                  const std::shared_ptr<LiveObject>& caller_live_object,
                  ObjectReferenceImpl* callee_object_reference,
                  const std::string& method_name,
                  const std::vector<Value>& parameters,
                  Value* return_value) override;
  bool ObjectsAreIdentical(const ObjectReferenceImpl* a,
                           const ObjectReferenceImpl* b) const override;

  bool CallMethodHelper(ObjectReferenceImpl* callee_object_reference,
                        const std::string& method_name,
                        const std::vector<Value>& parameters,
                        Value* return_value,
                        std::shared_ptr<LiveObject>* callee_live_object);

  // These methods take ownership of the CommittedEvent instances.
  void AddTransactionEvent(
      ObjectReferenceImpl* event_object_reference,
      CommittedEvent* event,
      ObjectReferenceImpl* prev_object_reference,
      const std::shared_ptr<LiveObject>& prev_live_object);
  // TODO(dss): The API of this function is horrible.
  void AddTransactionEvents(
      const std::unordered_map<ObjectReferenceImpl*,
                               std::vector<CommittedEvent*>>& object_events,
      ObjectReferenceImpl* prev_object_reference,
      const std::shared_ptr<LiveObject>& prev_live_object);

  void CommitTransaction();

  bool Rewinding();

  TransactionStoreInternalInterface* const transaction_store_;

  std::unique_ptr<PendingTransaction> pending_transaction_;

  DISALLOW_COPY_AND_ASSIGN(RecordingThread);
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_RECORDING_THREAD_H_
