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

#include "engine/committed_event.h"

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "base/escape.h"
#include "base/logging.h"
#include "engine/live_object.h"
#include "engine/object_reference_impl.h"
#include "include/c++/value.h"
#include "util/dump_context.h"
#include "util/stl_util.h"

using std::shared_ptr;
using std::string;
using std::unordered_set;
using std::vector;

namespace floating_temple {
namespace engine {

CommittedEvent::CommittedEvent(
    const unordered_set<ObjectReferenceImpl*>& new_objects)
    : new_objects_(new_objects) {
  CHECK(new_objects.find(nullptr) == new_objects.end());
}

void CommittedEvent::GetObjectCreation(
    shared_ptr<const LiveObject>* live_object) const {
  LOG(FATAL) << "Invalid call to GetObjectCreation (type == "
             << static_cast<int>(this->type()) << ")";
}

void CommittedEvent::GetMethodCall(const string** method_name,
                                   const vector<Value>** parameters) const {
  LOG(FATAL) << "Invalid call to GetMethodCall (type == "
             << static_cast<int>(this->type()) << ")";
}

void CommittedEvent::GetMethodReturn(const Value** return_value) const {
  LOG(FATAL) << "Invalid call to GetMethodReturn (type == "
             << static_cast<int>(this->type()) << ")";
}

void CommittedEvent::GetSubMethodCall(ObjectReferenceImpl** callee,
                                      const string** method_name,
                                      const vector<Value>** parameters) const {
  LOG(FATAL) << "Invalid call to GetSubMethodCall (type == "
             << static_cast<int>(this->type()) << ")";
}

void CommittedEvent::GetSubMethodReturn(const Value** return_value) const {
  LOG(FATAL) << "Invalid call to GetSubMethodReturn (type == "
             << static_cast<int>(this->type()) << ")";
}

void CommittedEvent::GetSelfMethodCall(const string** method_name,
                                       const vector<Value>** parameters) const {
  LOG(FATAL) << "Invalid call to GetSelfMethodCall (type == "
             << static_cast<int>(this->type()) << ")";
}

void CommittedEvent::GetSelfMethodReturn(const Value** return_value) const {
  LOG(FATAL) << "Invalid call to GetSelfMethodReturn (type == "
             << static_cast<int>(this->type()) << ")";
}

#define CHECK_EVENT_TYPE(event_type) \
  case CommittedEvent::event_type: \
    do { \
      return #event_type; \
    } while (false)

// static
string CommittedEvent::GetTypeString(Type event_type) {
  switch (event_type) {
    CHECK_EVENT_TYPE(OBJECT_CREATION);
    CHECK_EVENT_TYPE(BEGIN_TRANSACTION);
    CHECK_EVENT_TYPE(END_TRANSACTION);
    CHECK_EVENT_TYPE(METHOD_CALL);
    CHECK_EVENT_TYPE(METHOD_RETURN);
    CHECK_EVENT_TYPE(SUB_METHOD_CALL);
    CHECK_EVENT_TYPE(SUB_METHOD_RETURN);
    CHECK_EVENT_TYPE(SELF_METHOD_CALL);
    CHECK_EVENT_TYPE(SELF_METHOD_RETURN);

    default:
      LOG(FATAL) << "Invalid event type: " << static_cast<int>(event_type);
  }
}

#undef CHECK_EVENT_TYPE

void CommittedEvent::DumpNewObjects(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginList();
  for (const ObjectReferenceImpl* const object_reference : new_objects_) {
    object_reference->Dump(dc);
  }
  dc->End();
}

ObjectCreationCommittedEvent::ObjectCreationCommittedEvent(
    const shared_ptr<const LiveObject>& live_object)
    : CommittedEvent(unordered_set<ObjectReferenceImpl*>()),
      live_object_(live_object) {
  CHECK(live_object.get() != nullptr);
}

void ObjectCreationCommittedEvent::GetObjectCreation(
    shared_ptr<const LiveObject>* live_object) const {
  CHECK(live_object != nullptr);
  *live_object = live_object_;
}

CommittedEvent* ObjectCreationCommittedEvent::Clone() const {
  return new ObjectCreationCommittedEvent(live_object_);
}

void ObjectCreationCommittedEvent::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("OBJECT_CREATION");

  dc->AddString("new_objects");
  DumpNewObjects(dc);

  dc->AddString("live_object");
  live_object_->Dump(dc);

  dc->End();
}

BeginTransactionCommittedEvent::BeginTransactionCommittedEvent()
    : CommittedEvent(unordered_set<ObjectReferenceImpl*>()) {
}

CommittedEvent* BeginTransactionCommittedEvent::Clone() const {
  return new BeginTransactionCommittedEvent();
}

void BeginTransactionCommittedEvent::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("BEGIN_TRANSACTION");

  dc->AddString("new_objects");
  DumpNewObjects(dc);

  dc->End();
}

EndTransactionCommittedEvent::EndTransactionCommittedEvent()
    : CommittedEvent(unordered_set<ObjectReferenceImpl*>()) {
}

CommittedEvent* EndTransactionCommittedEvent::Clone() const {
  return new EndTransactionCommittedEvent();
}

void EndTransactionCommittedEvent::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("END_TRANSACTION");

  dc->AddString("new_objects");
  DumpNewObjects(dc);

  dc->End();
}

MethodCallCommittedEvent::MethodCallCommittedEvent(
    const string& method_name, const vector<Value>& parameters)
    : CommittedEvent(unordered_set<ObjectReferenceImpl*>()),
      method_name_(method_name),
      parameters_(parameters) {
  CHECK(!method_name.empty());
}

void MethodCallCommittedEvent::GetMethodCall(
    const string** method_name, const vector<Value>** parameters) const {
  CHECK(method_name != nullptr);
  CHECK(parameters != nullptr);

  *method_name = &method_name_;
  *parameters = &parameters_;
}

CommittedEvent* MethodCallCommittedEvent::Clone() const {
  return new MethodCallCommittedEvent(method_name_, parameters_);
}

void MethodCallCommittedEvent::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("METHOD_CALL");

  dc->AddString("new_objects");
  DumpNewObjects(dc);

  dc->AddString("method_name");
  dc->AddString(method_name_);

  dc->AddString("parameters");
  dc->BeginList();
  for (const Value& parameter : parameters_) {
    parameter.Dump(dc);
  }
  dc->End();

  dc->End();
}

MethodReturnCommittedEvent::MethodReturnCommittedEvent(
    const unordered_set<ObjectReferenceImpl*>& new_objects,
    const Value& return_value)
    : CommittedEvent(new_objects),
      return_value_(return_value) {
}

void MethodReturnCommittedEvent::GetMethodReturn(
    const Value** return_value) const {
  CHECK(return_value != nullptr);
  *return_value = &return_value_;
}

CommittedEvent* MethodReturnCommittedEvent::Clone() const {
  return new MethodReturnCommittedEvent(new_objects(), return_value_);
}

void MethodReturnCommittedEvent::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("METHOD_RETURN");

  dc->AddString("new_objects");
  DumpNewObjects(dc);

  dc->AddString("return_value");
  return_value_.Dump(dc);

  dc->End();
}

SubMethodCallCommittedEvent::SubMethodCallCommittedEvent(
    const unordered_set<ObjectReferenceImpl*>& new_objects,
    ObjectReferenceImpl* callee,
    const string& method_name,
    const vector<Value>& parameters)
    : CommittedEvent(new_objects),
      callee_(CHECK_NOTNULL(callee)),
      method_name_(method_name),
      parameters_(parameters) {
  CHECK(!method_name.empty());
}

void SubMethodCallCommittedEvent::GetSubMethodCall(
    ObjectReferenceImpl** callee, const string** method_name,
    const vector<Value>** parameters) const {
  CHECK(callee != nullptr);
  CHECK(method_name != nullptr);
  CHECK(parameters != nullptr);

  *callee = callee_;
  *method_name = &method_name_;
  *parameters = &parameters_;
}

CommittedEvent* SubMethodCallCommittedEvent::Clone() const {
  return new SubMethodCallCommittedEvent(new_objects(), callee_, method_name_,
                                         parameters_);
}

void SubMethodCallCommittedEvent::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("SUB_METHOD_CALL");

  dc->AddString("new_objects");
  DumpNewObjects(dc);

  dc->AddString("callee");
  callee_->Dump(dc);

  dc->AddString("method_name");
  dc->AddString(method_name_);

  dc->AddString("parameters");
  dc->BeginList();
  for (const Value& parameter : parameters_) {
    parameter.Dump(dc);
  }
  dc->End();

  dc->End();
}

SubMethodReturnCommittedEvent::SubMethodReturnCommittedEvent(
    const Value& return_value)
    : CommittedEvent(unordered_set<ObjectReferenceImpl*>()),
      return_value_(return_value) {
}

void SubMethodReturnCommittedEvent::GetSubMethodReturn(
    const Value** return_value) const {
  CHECK(return_value != nullptr);
  *return_value = &return_value_;
}

CommittedEvent* SubMethodReturnCommittedEvent::Clone() const {
  return new SubMethodReturnCommittedEvent(return_value_);
}

void SubMethodReturnCommittedEvent::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("SUB_METHOD_RETURN");

  dc->AddString("new_objects");
  DumpNewObjects(dc);

  dc->AddString("return_value");
  return_value_.Dump(dc);

  dc->End();
}

SelfMethodCallCommittedEvent::SelfMethodCallCommittedEvent(
    const unordered_set<ObjectReferenceImpl*>& new_objects,
    const string& method_name, const vector<Value>& parameters)
    : CommittedEvent(new_objects),
      method_name_(method_name),
      parameters_(parameters) {
  CHECK(!method_name.empty());
}

void SelfMethodCallCommittedEvent::GetSelfMethodCall(
    const string** method_name,
    const vector<Value>** parameters) const {
  CHECK(method_name != nullptr);
  CHECK(parameters != nullptr);

  *method_name = &method_name_;
  *parameters = &parameters_;
}

CommittedEvent* SelfMethodCallCommittedEvent::Clone() const {
  return new SelfMethodCallCommittedEvent(new_objects(), method_name_,
                                          parameters_);
}

void SelfMethodCallCommittedEvent::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("SELF_METHOD_CALL");

  dc->AddString("new_objects");
  DumpNewObjects(dc);

  dc->AddString("method_name");
  dc->AddString(method_name_);

  dc->AddString("parameters");
  dc->BeginList();
  for (const Value& parameter : parameters_) {
    parameter.Dump(dc);
  }
  dc->End();

  dc->End();
}

SelfMethodReturnCommittedEvent::SelfMethodReturnCommittedEvent(
    const unordered_set<ObjectReferenceImpl*>& new_objects,
    const Value& return_value)
    : CommittedEvent(new_objects),
      return_value_(return_value) {
}

void SelfMethodReturnCommittedEvent::GetSelfMethodReturn(
    const Value** return_value) const {
  CHECK(return_value != nullptr);
  *return_value = &return_value_;
}

CommittedEvent* SelfMethodReturnCommittedEvent::Clone() const {
  return new SelfMethodReturnCommittedEvent(new_objects(), return_value_);
}

void SelfMethodReturnCommittedEvent::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("SELF_METHOD_RETURN");

  dc->AddString("new_objects");
  DumpNewObjects(dc);

  dc->AddString("return_value");
  return_value_.Dump(dc);

  dc->End();
}

}  // namespace engine
}  // namespace floating_temple
