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

#ifndef PEER_CONST_LIVE_OBJECT_PTR_H_
#define PEER_CONST_LIVE_OBJECT_PTR_H_

namespace floating_temple {
namespace peer {

class LiveObject;
class LiveObjectPtr;

class ConstLiveObjectPtr {
 public:
  explicit ConstLiveObjectPtr(LiveObject* live_object = nullptr);
  ConstLiveObjectPtr(const ConstLiveObjectPtr& other);
  ConstLiveObjectPtr(const LiveObjectPtr& other);
  ~ConstLiveObjectPtr();

  // live_object may be NULL.
  void reset(LiveObject* live_object);

  ConstLiveObjectPtr& operator=(const ConstLiveObjectPtr& other);
  ConstLiveObjectPtr& operator=(const LiveObjectPtr& other);

  const LiveObject& operator*() const;
  const LiveObject* operator->() const;

  const LiveObject* get() const { return live_object_; }

 private:
  void Assign(LiveObject* live_object);

  void IncrementRefCount();
  void DecrementRefCount();

  // May be NULL.
  LiveObject* live_object_;
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_CONST_LIVE_OBJECT_PTR_H_
