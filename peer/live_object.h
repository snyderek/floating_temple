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

#ifndef PEER_LIVE_OBJECT_H_
#define PEER_LIVE_OBJECT_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/mutex.h"
#include "include/c++/value.h"
#include "peer/live_object_ptr.h"

namespace floating_temple {

class LocalObject;
class Thread;

namespace peer {

class LiveObjectNode;
class PeerObjectImpl;

class LiveObject {
 public:
  explicit LiveObject(LocalObject* local_object);
  ~LiveObject();

  const LocalObject* local_object() const;

  LiveObjectPtr Clone() const;
  void Serialize(std::string* data,
                 std::vector<PeerObjectImpl*>* referenced_peer_objects) const;
  void InvokeMethod(Thread* thread,
                    PeerObjectImpl* peer_object,
                    const std::string& method_name,
                    const std::vector<Value>& parameters,
                    Value* return_value);

  std::string Dump() const;

  void IncrementRefCount();
  bool DecrementRefCount();

 private:
  explicit LiveObject(LiveObjectNode* node);

  LiveObjectNode* GetNode() const;

  LiveObjectNode* node_;  // Not NULL
  mutable Mutex node_mu_;

  int ref_count_;
  mutable Mutex ref_count_mu_;

  DISALLOW_COPY_AND_ASSIGN(LiveObject);
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_LIVE_OBJECT_H_
