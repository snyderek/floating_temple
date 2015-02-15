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

#ifndef PEER_LIVE_OBJECT_PTR_H_
#define PEER_LIVE_OBJECT_PTR_H_

#include <cstddef>

namespace floating_temple {
namespace peer {

class ConstLiveObjectPtr;
class LiveObject;

class LiveObjectPtr {
 public:
  explicit LiveObjectPtr(LiveObject* live_object = NULL);
  LiveObjectPtr(const LiveObjectPtr& other);
  ~LiveObjectPtr();

  // live_object may be NULL.
  void reset(LiveObject* live_object);

  LiveObjectPtr& operator=(const LiveObjectPtr& other);

  LiveObject& operator*() const;
  LiveObject* operator->() const;

  const LiveObject* get() const { return live_object_; }

 private:
  void Assign(LiveObject* live_object);

  void IncrementRefCount();
  void DecrementRefCount();

  // May be NULL.
  LiveObject* live_object_;

  friend class ConstLiveObjectPtr;
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_LIVE_OBJECT_PTR_H_
