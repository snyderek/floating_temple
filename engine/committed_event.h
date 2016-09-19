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
#include <vector>

#include "base/macros.h"
#include "include/c++/value.h"

namespace floating_temple {

class DumpContext;

namespace engine {

class LiveObject;
class ObjectReferenceImpl;

// TODO(dss): Rename this class to Event. It's no longer used just for committed
// events.
class CommittedEvent {
 public:
  enum Type {
    // <Object> was created.
    OBJECT_CREATION,

    // <Object> created another object.
    SUB_OBJECT_CREATION,

    // <Object> began a transaction (possibly nested).
    BEGIN_TRANSACTION,

    // <Object> ended the current transaction (possibly nested).
    END_TRANSACTION,

    // A method was called on <Object>.
    METHOD_CALL,

    // A method on <Object> returned.
    METHOD_RETURN,

    // <Object> called a method on another object.
    SUB_METHOD_CALL,

    // A method on another object returned to <Object>.
    SUB_METHOD_RETURN,

    // <Object> called a method on itself.
    SELF_METHOD_CALL,

    // A self-method call on <Object> returned.
    SELF_METHOD_RETURN
  };

  CommittedEvent() {}
  virtual ~CommittedEvent() {}

  virtual Type type() const = 0;

  virtual void GetObjectCreation(
      std::shared_ptr<const LiveObject>* live_object) const;
  virtual void GetSubObjectCreation(const std::string** new_object_name,
                                    ObjectReferenceImpl** new_object) const;
  virtual void GetMethodCall(const std::string** method_name,
                             const std::vector<Value>** parameters) const;
  virtual void GetMethodReturn(const Value** return_value) const;
  virtual void GetSubMethodCall(ObjectReferenceImpl** callee,
                                const std::string** method_name,
                                const std::vector<Value>** parameters) const;
  virtual void GetSubMethodReturn(const Value** return_value) const;
  virtual void GetSelfMethodCall(const std::string** method_name,
                                 const std::vector<Value>** parameters) const;
  virtual void GetSelfMethodReturn(const Value** return_value) const;

  virtual CommittedEvent* Clone() const = 0;

  virtual std::string DebugString() const;
  virtual void Dump(DumpContext* dc) const = 0;

  static std::string GetTypeString(Type event_type);
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

class SubObjectCreationCommittedEvent : public CommittedEvent {
 public:
  SubObjectCreationCommittedEvent(const std::string& new_object_name,
                                  ObjectReferenceImpl* new_object);

  Type type() const override { return SUB_OBJECT_CREATION; }
  void GetSubObjectCreation(const std::string** new_object_name,
                            ObjectReferenceImpl** new_object) const override;
  CommittedEvent* Clone() const override;
  void Dump(DumpContext* dc) const override;

 private:
  const std::string new_object_name_;
  ObjectReferenceImpl* const new_object_;

  DISALLOW_COPY_AND_ASSIGN(SubObjectCreationCommittedEvent);
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
  MethodCallCommittedEvent(const std::string& method_name,
                           const std::vector<Value>& parameters);

  Type type() const override { return METHOD_CALL; }
  void GetMethodCall(const std::string** method_name,
                     const std::vector<Value>** parameters) const override;
  CommittedEvent* Clone() const override;
  std::string DebugString() const override;
  void Dump(DumpContext* dc) const override;

 private:
  const std::string method_name_;
  const std::vector<Value> parameters_;

  DISALLOW_COPY_AND_ASSIGN(MethodCallCommittedEvent);
};

class MethodReturnCommittedEvent : public CommittedEvent {
 public:
  explicit MethodReturnCommittedEvent(const Value& return_value);

  Type type() const override { return METHOD_RETURN; }
  void GetMethodReturn(const Value** return_value) const override;
  CommittedEvent* Clone() const override;
  void Dump(DumpContext* dc) const override;

 private:
  const Value return_value_;

  DISALLOW_COPY_AND_ASSIGN(MethodReturnCommittedEvent);
};

class SubMethodCallCommittedEvent : public CommittedEvent {
 public:
  SubMethodCallCommittedEvent(ObjectReferenceImpl* callee,
                              const std::string& method_name,
                              const std::vector<Value>& parameters);

  Type type() const override { return SUB_METHOD_CALL; }
  void GetSubMethodCall(ObjectReferenceImpl** callee,
                        const std::string** method_name,
                        const std::vector<Value>** parameters) const override;
  CommittedEvent* Clone() const override;
  std::string DebugString() const override;
  void Dump(DumpContext* dc) const override;

 private:
  ObjectReferenceImpl* const callee_;
  const std::string method_name_;
  const std::vector<Value> parameters_;

  DISALLOW_COPY_AND_ASSIGN(SubMethodCallCommittedEvent);
};

class SubMethodReturnCommittedEvent : public CommittedEvent {
 public:
  explicit SubMethodReturnCommittedEvent(const Value& return_value);

  Type type() const override { return SUB_METHOD_RETURN; }
  void GetSubMethodReturn(const Value** return_value) const override;
  CommittedEvent* Clone() const override;
  void Dump(DumpContext* dc) const override;

 private:
  const Value return_value_;

  DISALLOW_COPY_AND_ASSIGN(SubMethodReturnCommittedEvent);
};

class SelfMethodCallCommittedEvent : public CommittedEvent {
 public:
  SelfMethodCallCommittedEvent(const std::string& method_name,
                               const std::vector<Value>& parameters);

  Type type() const override { return SELF_METHOD_CALL; }
  void GetSelfMethodCall(const std::string** method_name,
                         const std::vector<Value>** parameters) const override;
  CommittedEvent* Clone() const override;
  std::string DebugString() const override;
  void Dump(DumpContext* dc) const override;

 private:
  const std::string method_name_;
  const std::vector<Value> parameters_;

  DISALLOW_COPY_AND_ASSIGN(SelfMethodCallCommittedEvent);
};

class SelfMethodReturnCommittedEvent : public CommittedEvent {
 public:
  explicit SelfMethodReturnCommittedEvent(const Value& return_value);

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
