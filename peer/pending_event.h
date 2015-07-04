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

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "base/macros.h"
#include "include/c++/value.h"

namespace floating_temple {
namespace peer {

class LiveObject;
class ObjectReferenceImpl;

class PendingEvent {
 public:
  enum Type {
    OBJECT_CREATION,
    BEGIN_TRANSACTION,
    END_TRANSACTION,
    METHOD_CALL,
    METHOD_RETURN
  };

  // new_object_references must be a subset of keys(live_objects).
  PendingEvent(
      const std::unordered_map<ObjectReferenceImpl*,
                               std::shared_ptr<const LiveObject>>& live_objects,
      const std::unordered_set<ObjectReferenceImpl*>& new_object_references,
      ObjectReferenceImpl* prev_object_reference);
  virtual ~PendingEvent() {}

  const std::unordered_map<ObjectReferenceImpl*,
                           std::shared_ptr<const LiveObject>>&
      live_objects() const { return live_objects_; }
  const std::unordered_set<ObjectReferenceImpl*>& new_object_references() const
      { return new_object_references_; }
  ObjectReferenceImpl* prev_object_reference() const
      { return prev_object_reference_; }

  virtual Type type() const = 0;

  virtual void GetMethodCall(ObjectReferenceImpl** next_object_reference,
                             const std::string** method_name,
                             const std::vector<Value>** parameters) const;
  virtual void GetMethodReturn(ObjectReferenceImpl** next_object_reference,
                               const Value** return_value) const;

 private:
  const std::unordered_map<ObjectReferenceImpl*,
                           std::shared_ptr<const LiveObject>> live_objects_;
  const std::unordered_set<ObjectReferenceImpl*> new_object_references_;
  ObjectReferenceImpl* const prev_object_reference_;
};

class ObjectCreationPendingEvent : public PendingEvent {
 public:
  // prev_object_reference may be NULL.
  ObjectCreationPendingEvent(
      ObjectReferenceImpl* prev_object_reference,
      ObjectReferenceImpl* new_object_reference,
      const std::shared_ptr<const LiveObject>& new_live_object);

  Type type() const override { return OBJECT_CREATION; }

 private:
  DISALLOW_COPY_AND_ASSIGN(ObjectCreationPendingEvent);
};

class BeginTransactionPendingEvent : public PendingEvent {
 public:
  // prev_object_reference must not be NULL.
  explicit BeginTransactionPendingEvent(
      ObjectReferenceImpl* prev_object_reference);

  Type type() const override { return BEGIN_TRANSACTION; }

 private:
  DISALLOW_COPY_AND_ASSIGN(BeginTransactionPendingEvent);
};

class EndTransactionPendingEvent : public PendingEvent {
 public:
  // prev_object_reference must not be NULL.
  explicit EndTransactionPendingEvent(
      ObjectReferenceImpl* prev_object_reference);

  Type type() const override { return END_TRANSACTION; }

 private:
  DISALLOW_COPY_AND_ASSIGN(EndTransactionPendingEvent);
};

class MethodCallPendingEvent : public PendingEvent {
 public:
  // prev_object_reference may be NULL.
  MethodCallPendingEvent(
      const std::unordered_map<ObjectReferenceImpl*,
                               std::shared_ptr<const LiveObject>>& live_objects,
      const std::unordered_set<ObjectReferenceImpl*>& new_object_references,
      ObjectReferenceImpl* prev_object_reference,
      ObjectReferenceImpl* next_object_reference,
      const std::string& method_name,
      const std::vector<Value>& parameters);

  Type type() const override { return METHOD_CALL; }
  void GetMethodCall(ObjectReferenceImpl** next_object_reference,
                     const std::string** method_name,
                     const std::vector<Value>** parameters) const override;

 private:
  ObjectReferenceImpl* const next_object_reference_;
  const std::string method_name_;
  const std::vector<Value> parameters_;

  DISALLOW_COPY_AND_ASSIGN(MethodCallPendingEvent);
};

class MethodReturnPendingEvent : public PendingEvent {
 public:
  // prev_object_reference must not be NULL.
  MethodReturnPendingEvent(
      const std::unordered_map<ObjectReferenceImpl*,
                               std::shared_ptr<const LiveObject>>& live_objects,
      const std::unordered_set<ObjectReferenceImpl*>& new_object_references,
      ObjectReferenceImpl* prev_object_reference,
      ObjectReferenceImpl* next_object_reference,
      const Value& return_value);

  Type type() const override { return METHOD_RETURN; }
  void GetMethodReturn(ObjectReferenceImpl** next_object_reference,
                       const Value** return_value) const override;

 private:
  ObjectReferenceImpl* const next_object_reference_;
  const Value return_value_;

  DISALLOW_COPY_AND_ASSIGN(MethodReturnPendingEvent);
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_PENDING_EVENT_H_
