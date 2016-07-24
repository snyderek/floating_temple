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

#ifndef ENGINE_COMMITTED_EVENT_H_
#define ENGINE_COMMITTED_EVENT_H_

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "base/macros.h"
#include "include/c++/value.h"

namespace floating_temple {

class DumpContext;

namespace engine {

class LiveObject;
class SharedObject;

class CommittedEvent {
 public:
  enum Type {
    OBJECT_CREATION,
    BEGIN_TRANSACTION,
    END_TRANSACTION,
    METHOD_CALL,
    METHOD_RETURN,
    SUB_METHOD_CALL,
    SUB_METHOD_RETURN,
    SELF_METHOD_CALL,
    SELF_METHOD_RETURN
  };

  explicit CommittedEvent(
      const std::unordered_set<SharedObject*>& new_shared_objects);
  virtual ~CommittedEvent() {}

  const std::unordered_set<SharedObject*>& new_shared_objects() const
      { return new_shared_objects_; }

  virtual Type type() const = 0;

  virtual void GetObjectCreation(
      std::shared_ptr<const LiveObject>* live_object) const;
  virtual void GetMethodCall(SharedObject** caller,
                             const std::string** method_name,
                             const std::vector<Value>** parameters) const;
  virtual void GetMethodReturn(SharedObject** caller,
                               const Value** return_value) const;
  virtual void GetSubMethodCall(SharedObject** callee,
                                const std::string** method_name,
                                const std::vector<Value>** parameters) const;
  virtual void GetSubMethodReturn(SharedObject** callee,
                                  const Value** return_value) const;
  virtual void GetSelfMethodCall(const std::string** method_name,
                                 const std::vector<Value>** parameters) const;
  virtual void GetSelfMethodReturn(const Value** return_value) const;

  virtual CommittedEvent* Clone() const = 0;

  virtual void Dump(DumpContext* dc) const = 0;

  static std::string GetTypeString(Type event_type);

 protected:
  void DumpNewSharedObjects(DumpContext* dc) const;

 private:
  const std::unordered_set<SharedObject*> new_shared_objects_;
};

class ObjectCreationCommittedEvent : public CommittedEvent {
 public:
  explicit ObjectCreationCommittedEvent(
      const std::shared_ptr<const LiveObject>& live_object);

  Type type() const override { return OBJECT_CREATION; }
  void GetObjectCreation(
      std::shared_ptr<const LiveObject>* live_object) const override;
  CommittedEvent* Clone() const override;
  void Dump(DumpContext* dc) const override;

 private:
  const std::shared_ptr<const LiveObject> live_object_;

  DISALLOW_COPY_AND_ASSIGN(ObjectCreationCommittedEvent);
};

class BeginTransactionCommittedEvent : public CommittedEvent {
 public:
  BeginTransactionCommittedEvent();

  Type type() const override { return BEGIN_TRANSACTION; }
  CommittedEvent* Clone() const override;
  void Dump(DumpContext* dc) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BeginTransactionCommittedEvent);
};

class EndTransactionCommittedEvent : public CommittedEvent {
 public:
  EndTransactionCommittedEvent();

  Type type() const override { return END_TRANSACTION; }
  CommittedEvent* Clone() const override;
  void Dump(DumpContext* dc) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(EndTransactionCommittedEvent);
};

class MethodCallCommittedEvent : public CommittedEvent {
 public:
  MethodCallCommittedEvent(SharedObject* caller, const std::string& method_name,
                           const std::vector<Value>& parameters);

  Type type() const override { return METHOD_CALL; }
  void GetMethodCall(SharedObject** caller, const std::string** method_name,
                     const std::vector<Value>** parameters) const override;
  CommittedEvent* Clone() const override;
  void Dump(DumpContext* dc) const override;

 private:
  SharedObject* const caller_;
  const std::string method_name_;
  const std::vector<Value> parameters_;

  DISALLOW_COPY_AND_ASSIGN(MethodCallCommittedEvent);
};

class MethodReturnCommittedEvent : public CommittedEvent {
 public:
  MethodReturnCommittedEvent(
      const std::unordered_set<SharedObject*>& new_shared_objects,
      SharedObject* caller, const Value& return_value);

  Type type() const override { return METHOD_RETURN; }
  void GetMethodReturn(SharedObject** caller,
                       const Value** return_value) const override;
  CommittedEvent* Clone() const override;
  void Dump(DumpContext* dc) const override;

 private:
  SharedObject* const caller_;
  const Value return_value_;

  DISALLOW_COPY_AND_ASSIGN(MethodReturnCommittedEvent);
};

class SubMethodCallCommittedEvent : public CommittedEvent {
 public:
  SubMethodCallCommittedEvent(
      const std::unordered_set<SharedObject*>& new_shared_objects,
      SharedObject* callee,
      const std::string& method_name,
      const std::vector<Value>& parameters);

  Type type() const override { return SUB_METHOD_CALL; }
  void GetSubMethodCall(SharedObject** callee, const std::string** method_name,
                        const std::vector<Value>** parameters) const override;
  CommittedEvent* Clone() const override;
  void Dump(DumpContext* dc) const override;

 private:
  SharedObject* const callee_;
  const std::string method_name_;
  const std::vector<Value> parameters_;

  DISALLOW_COPY_AND_ASSIGN(SubMethodCallCommittedEvent);
};

class SubMethodReturnCommittedEvent : public CommittedEvent {
 public:
  SubMethodReturnCommittedEvent(SharedObject* callee,
                                const Value& return_value);

  Type type() const override { return SUB_METHOD_RETURN; }
  void GetSubMethodReturn(SharedObject** callee,
                          const Value** return_value) const override;
  CommittedEvent* Clone() const override;
  void Dump(DumpContext* dc) const override;

 private:
  SharedObject* const callee_;
  const Value return_value_;

  DISALLOW_COPY_AND_ASSIGN(SubMethodReturnCommittedEvent);
};

class SelfMethodCallCommittedEvent : public CommittedEvent {
 public:
  SelfMethodCallCommittedEvent(
      const std::unordered_set<SharedObject*>& new_shared_objects,
      const std::string& method_name, const std::vector<Value>& parameters);

  Type type() const override { return SELF_METHOD_CALL; }
  void GetSelfMethodCall(const std::string** method_name,
                         const std::vector<Value>** parameters) const override;
  CommittedEvent* Clone() const override;
  void Dump(DumpContext* dc) const override;

 private:
  const std::string method_name_;
  const std::vector<Value> parameters_;

  DISALLOW_COPY_AND_ASSIGN(SelfMethodCallCommittedEvent);
};

class SelfMethodReturnCommittedEvent : public CommittedEvent {
 public:
  SelfMethodReturnCommittedEvent(
      const std::unordered_set<SharedObject*>& new_shared_objects,
      const Value& return_value);

  Type type() const override { return SELF_METHOD_RETURN; }
  void GetSelfMethodReturn(const Value** return_value) const override;
  CommittedEvent* Clone() const override;
  void Dump(DumpContext* dc) const override;

 private:
  const Value return_value_;

  DISALLOW_COPY_AND_ASSIGN(SelfMethodReturnCommittedEvent);
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_COMMITTED_EVENT_H_
