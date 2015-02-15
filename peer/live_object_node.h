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

#ifndef PEER_LIVE_OBJECT_NODE_H_
#define PEER_LIVE_OBJECT_NODE_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/mutex.h"
#include "include/c++/value.h"

namespace floating_temple {

class LocalObject;
class Thread;

namespace peer {

class PeerObjectImpl;

class LiveObjectNode {
 public:
  explicit LiveObjectNode(LocalObject* local_object);
  ~LiveObjectNode();

  const LocalObject* local_object() const { return local_object_; }

  void Serialize(std::string* data,
                 std::vector<PeerObjectImpl*>* referenced_peer_objects) const;
  LiveObjectNode* InvokeMethod(Thread* thread,
                               PeerObjectImpl* peer_object,
                               const std::string& method_name,
                               const std::vector<Value>& parameters,
                               Value* return_value);
  std::string Dump() const;

  void IncrementRefCount();
  bool DecrementRefCount();

 private:
  int GetRefCount() const;

  LocalObject* const local_object_;  // Not NULL

  int ref_count_;
  mutable Mutex ref_count_mu_;

  DISALLOW_COPY_AND_ASSIGN(LiveObjectNode);
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_LIVE_OBJECT_NODE_H_
