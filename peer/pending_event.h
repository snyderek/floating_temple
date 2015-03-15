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

#ifndef PEER_PENDING_EVENT_H_
#define PEER_PENDING_EVENT_H_

#include <string>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include <vector>

#include "base/macros.h"
#include "include/c++/value.h"
#include "peer/const_live_object_ptr.h"

namespace floating_temple {
namespace peer {

class PeerObjectImpl;

class PendingEvent {
 public:
  enum Type {
    OBJECT_CREATION,
    BEGIN_TRANSACTION,
    END_TRANSACTION,
    METHOD_CALL,
    METHOD_RETURN
  };

  // new_peer_objects must be a subset of keys(live_objects).
  PendingEvent(
      const std::tr1::unordered_map<PeerObjectImpl*, ConstLiveObjectPtr>&
          live_objects,
      const std::tr1::unordered_set<PeerObjectImpl*>& new_peer_objects,
      PeerObjectImpl* prev_peer_object);
  virtual ~PendingEvent() {}

  const std::tr1::unordered_map<PeerObjectImpl*, ConstLiveObjectPtr>&
      live_objects() const { return live_objects_; }
  const std::tr1::unordered_set<PeerObjectImpl*>& new_peer_objects() const
      { return new_peer_objects_; }
  PeerObjectImpl* prev_peer_object() const { return prev_peer_object_; }

  virtual Type type() const = 0;

  virtual void GetMethodCall(PeerObjectImpl** next_peer_object,
                             const std::string** method_name,
                             const std::vector<Value>** parameters) const;
  virtual void GetMethodReturn(PeerObjectImpl** next_peer_object,
                               const Value** return_value) const;

 private:
  const std::tr1::unordered_map<PeerObjectImpl*, ConstLiveObjectPtr>
      live_objects_;
  const std::tr1::unordered_set<PeerObjectImpl*> new_peer_objects_;
  PeerObjectImpl* const prev_peer_object_;
};

class ObjectCreationPendingEvent : public PendingEvent {
 public:
  // prev_peer_object may be NULL.
  ObjectCreationPendingEvent(PeerObjectImpl* prev_peer_object,
                             PeerObjectImpl* new_peer_object,
                             const ConstLiveObjectPtr& new_live_object);

  virtual Type type() const { return OBJECT_CREATION; }

 private:
  DISALLOW_COPY_AND_ASSIGN(ObjectCreationPendingEvent);
};

class BeginTransactionPendingEvent : public PendingEvent {
 public:
  // prev_peer_object must not be NULL.
  explicit BeginTransactionPendingEvent(PeerObjectImpl* prev_peer_object);

  virtual Type type() const { return BEGIN_TRANSACTION; }

 private:
  DISALLOW_COPY_AND_ASSIGN(BeginTransactionPendingEvent);
};

class EndTransactionPendingEvent : public PendingEvent {
 public:
  // prev_peer_object must not be NULL.
  explicit EndTransactionPendingEvent(PeerObjectImpl* prev_peer_object);

  virtual Type type() const { return END_TRANSACTION; }

 private:
  DISALLOW_COPY_AND_ASSIGN(EndTransactionPendingEvent);
};

class MethodCallPendingEvent : public PendingEvent {
 public:
  // prev_peer_object may be NULL.
  MethodCallPendingEvent(
      const std::tr1::unordered_map<PeerObjectImpl*, ConstLiveObjectPtr>&
          live_objects,
      const std::tr1::unordered_set<PeerObjectImpl*>& new_peer_objects,
      PeerObjectImpl* prev_peer_object,
      PeerObjectImpl* next_peer_object,
      const std::string& method_name,
      const std::vector<Value>& parameters);

  virtual Type type() const { return METHOD_CALL; }
  virtual void GetMethodCall(PeerObjectImpl** next_peer_object,
                             const std::string** method_name,
                             const std::vector<Value>** parameters) const;

 private:
  PeerObjectImpl* const next_peer_object_;
  const std::string method_name_;
  const std::vector<Value> parameters_;

  DISALLOW_COPY_AND_ASSIGN(MethodCallPendingEvent);
};

class MethodReturnPendingEvent : public PendingEvent {
 public:
  // prev_peer_object must not be NULL.
  MethodReturnPendingEvent(
      const std::tr1::unordered_map<PeerObjectImpl*, ConstLiveObjectPtr>&
          live_objects,
      const std::tr1::unordered_set<PeerObjectImpl*>& new_peer_objects,
      PeerObjectImpl* prev_peer_object,
      PeerObjectImpl* next_peer_object,
      const Value& return_value);

  virtual Type type() const { return METHOD_RETURN; }
  virtual void GetMethodReturn(PeerObjectImpl** next_peer_object,
                               const Value** return_value) const;

 private:
  PeerObjectImpl* const next_peer_object_;
  const Value return_value_;

  DISALLOW_COPY_AND_ASSIGN(MethodReturnPendingEvent);
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_PENDING_EVENT_H_
