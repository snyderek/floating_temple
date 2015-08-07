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
#include "engine/committed_value.h"
#include "engine/live_object.h"
#include "engine/shared_object.h"
#include "engine/uuid_util.h"
#include "util/dump_context.h"
#include "util/stl_util.h"

using std::shared_ptr;
using std::string;
using std::unordered_set;
using std::vector;

namespace floating_temple {
namespace engine {

CommittedEvent::CommittedEvent(
    const unordered_set<SharedObject*>& new_shared_objects)
    : new_shared_objects_(new_shared_objects) {
  CHECK(new_shared_objects.find(nullptr) == new_shared_objects.end());
}

void CommittedEvent::GetObjectCreation(
    shared_ptr<const LiveObject>* live_object) const {
  LOG(FATAL) << "Invalid call to GetObjectCreation (type == "
             << static_cast<int>(this->type()) << ")";
}

void CommittedEvent::GetMethodCall(
    SharedObject** caller, const string** method_name,
    const vector<CommittedValue>** parameters) const {
  LOG(FATAL) << "Invalid call to GetMethodCall (type == "
             << static_cast<int>(this->type()) << ")";
}

void CommittedEvent::GetMethodReturn(
    SharedObject** caller, const CommittedValue** return_value) const {
  LOG(FATAL) << "Invalid call to GetMethodReturn (type == "
             << static_cast<int>(this->type()) << ")";
}

void CommittedEvent::GetSubMethodCall(
    SharedObject** callee, const string** method_name,
    const vector<CommittedValue>** parameters) const {
  LOG(FATAL) << "Invalid call to GetSubMethodCall (type == "
             << static_cast<int>(this->type()) << ")";
}

void CommittedEvent::GetSubMethodReturn(
    SharedObject** callee, const CommittedValue** return_value) const {
  LOG(FATAL) << "Invalid call to GetSubMethodReturn (type == "
             << static_cast<int>(this->type()) << ")";
}

void CommittedEvent::GetSelfMethodCall(
    const string** method_name,
    const vector<CommittedValue>** parameters) const {
  LOG(FATAL) << "Invalid call to GetSelfMethodCall (type == "
             << static_cast<int>(this->type()) << ")";
}

void CommittedEvent::GetSelfMethodReturn(
    const CommittedValue** return_value) const {
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
    CHECK_EVENT_TYPE(SUB_OBJECT_CREATION);
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

  return "";
}

#undef CHECK_EVENT_TYPE

void CommittedEvent::DumpNewSharedObjects(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginList();
  for (const SharedObject* const shared_object : new_shared_objects_) {
    dc->AddString(UuidToString(shared_object->object_id()));
  }
  dc->End();
}

ObjectCreationCommittedEvent::ObjectCreationCommittedEvent(
    const shared_ptr<const LiveObject>& live_object)
    : CommittedEvent(unordered_set<SharedObject*>()),
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

  dc->AddString("new_shared_objects");
  DumpNewSharedObjects(dc);

  dc->AddString("live_object");
  live_object_->Dump(dc);

  dc->End();
}

SubObjectCreationCommittedEvent::SubObjectCreationCommittedEvent(
    SharedObject* new_shared_object)
    : CommittedEvent(
          MakeSingletonSet<unordered_set<SharedObject*>>(
                CHECK_NOTNULL(new_shared_object))) {
}

CommittedEvent* SubObjectCreationCommittedEvent::Clone() const {
  return new SubObjectCreationCommittedEvent(GetNewSharedObject());
}

void SubObjectCreationCommittedEvent::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("SUB_OBJECT_CREATION");

  dc->AddString("new_shared_object");
  dc->AddString(UuidToString(GetNewSharedObject()->object_id()));

  dc->End();
}

SharedObject* SubObjectCreationCommittedEvent::GetNewSharedObject() const {
  CHECK_EQ(new_shared_objects().size(), 1u);
  return *new_shared_objects().begin();
}

BeginTransactionCommittedEvent::BeginTransactionCommittedEvent()
    : CommittedEvent(unordered_set<SharedObject*>()) {
}

CommittedEvent* BeginTransactionCommittedEvent::Clone() const {
  return new BeginTransactionCommittedEvent();
}

void BeginTransactionCommittedEvent::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("BEGIN_TRANSACTION");

  dc->AddString("new_shared_objects");
  DumpNewSharedObjects(dc);

  dc->End();
}

EndTransactionCommittedEvent::EndTransactionCommittedEvent()
    : CommittedEvent(unordered_set<SharedObject*>()) {
}

CommittedEvent* EndTransactionCommittedEvent::Clone() const {
  return new EndTransactionCommittedEvent();
}

void EndTransactionCommittedEvent::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("END_TRANSACTION");

  dc->AddString("new_shared_objects");
  DumpNewSharedObjects(dc);

  dc->End();
}

MethodCallCommittedEvent::MethodCallCommittedEvent(
    SharedObject* caller, const string& method_name,
    const vector<CommittedValue>& parameters)
    : CommittedEvent(unordered_set<SharedObject*>()),
      caller_(caller),
      method_name_(method_name),
      parameters_(parameters) {
  CHECK(!method_name.empty());
}

void MethodCallCommittedEvent::GetMethodCall(
    SharedObject** caller, const string** method_name,
    const vector<CommittedValue>** parameters) const {
  CHECK(caller != nullptr);
  CHECK(method_name != nullptr);
  CHECK(parameters != nullptr);

  *caller = caller_;
  *method_name = &method_name_;
  *parameters = &parameters_;
}

CommittedEvent* MethodCallCommittedEvent::Clone() const {
  return new MethodCallCommittedEvent(caller_, method_name_, parameters_);
}

void MethodCallCommittedEvent::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("METHOD_CALL");

  dc->AddString("new_shared_objects");
  DumpNewSharedObjects(dc);

  dc->AddString("caller");
  if (caller_== nullptr) {
    dc->AddNull();
  } else {
    dc->AddString(UuidToString(caller_->object_id()));
  }

  dc->AddString("method_name");
  dc->AddString(method_name_);

  dc->AddString("parameters");
  dc->BeginList();
  for (const CommittedValue& parameter : parameters_) {
    parameter.Dump(dc);
  }
  dc->End();

  dc->End();
}

MethodReturnCommittedEvent::MethodReturnCommittedEvent(
    const unordered_set<SharedObject*>& new_shared_objects,
    SharedObject* caller,
    const CommittedValue& return_value)
    : CommittedEvent(new_shared_objects),
      caller_(caller),
      return_value_(return_value) {
}

void MethodReturnCommittedEvent::GetMethodReturn(
    SharedObject** caller, const CommittedValue** return_value) const {
  CHECK(caller != nullptr);
  CHECK(return_value != nullptr);

  *caller = caller_;
  *return_value = &return_value_;
}

CommittedEvent* MethodReturnCommittedEvent::Clone() const {
  return new MethodReturnCommittedEvent(new_shared_objects(), caller_,
                                        return_value_);
}

void MethodReturnCommittedEvent::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("METHOD_RETURN");

  dc->AddString("new_shared_objects");
  DumpNewSharedObjects(dc);

  dc->AddString("caller");
  if (caller_== nullptr) {
    dc->AddNull();
  } else {
    dc->AddString(UuidToString(caller_->object_id()));
  }

  dc->AddString("return_value");
  return_value_.Dump(dc);

  dc->End();
}

SubMethodCallCommittedEvent::SubMethodCallCommittedEvent(
    const unordered_set<SharedObject*>& new_shared_objects,
    SharedObject* callee,
    const string& method_name,
    const vector<CommittedValue>& parameters)
    : CommittedEvent(new_shared_objects),
      callee_(CHECK_NOTNULL(callee)),
      method_name_(method_name),
      parameters_(parameters) {
  CHECK(!method_name.empty());
}

void SubMethodCallCommittedEvent::GetSubMethodCall(
    SharedObject** callee, const string** method_name,
    const vector<CommittedValue>** parameters) const {
  CHECK(callee != nullptr);
  CHECK(method_name != nullptr);
  CHECK(parameters != nullptr);

  *callee = callee_;
  *method_name = &method_name_;
  *parameters = &parameters_;
}

CommittedEvent* SubMethodCallCommittedEvent::Clone() const {
  return new SubMethodCallCommittedEvent(new_shared_objects(), callee_,
                                         method_name_, parameters_);
}

void SubMethodCallCommittedEvent::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("SUB_METHOD_CALL");

  dc->AddString("new_shared_objects");
  DumpNewSharedObjects(dc);

  dc->AddString("callee");
  dc->AddString(UuidToString(callee_->object_id()));

  dc->AddString("method_name");
  dc->AddString(method_name_);

  dc->AddString("parameters");
  dc->BeginList();
  for (const CommittedValue& parameter : parameters_) {
    parameter.Dump(dc);
  }
  dc->End();

  dc->End();
}

SubMethodReturnCommittedEvent::SubMethodReturnCommittedEvent(
    SharedObject* callee, const CommittedValue& return_value)
    : CommittedEvent(unordered_set<SharedObject*>()),
      callee_(CHECK_NOTNULL(callee)),
      return_value_(return_value) {
}

void SubMethodReturnCommittedEvent::GetSubMethodReturn(
    SharedObject** callee, const CommittedValue** return_value) const {
  CHECK(callee != nullptr);
  CHECK(return_value != nullptr);

  *callee = callee_;
  *return_value = &return_value_;
}

CommittedEvent* SubMethodReturnCommittedEvent::Clone() const {
  return new SubMethodReturnCommittedEvent(callee_, return_value_);
}

void SubMethodReturnCommittedEvent::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("SUB_METHOD_RETURN");

  dc->AddString("new_shared_objects");
  DumpNewSharedObjects(dc);

  dc->AddString("callee");
  dc->AddString(UuidToString(callee_->object_id()));

  dc->AddString("return_value");
  return_value_.Dump(dc);

  dc->End();
}

SelfMethodCallCommittedEvent::SelfMethodCallCommittedEvent(
    const unordered_set<SharedObject*>& new_shared_objects,
    const string& method_name,
    const vector<CommittedValue>& parameters)
    : CommittedEvent(new_shared_objects),
      method_name_(method_name),
      parameters_(parameters) {
  CHECK(!method_name.empty());
}

void SelfMethodCallCommittedEvent::GetSelfMethodCall(
    const string** method_name,
    const vector<CommittedValue>** parameters) const {
  CHECK(method_name != nullptr);
  CHECK(parameters != nullptr);

  *method_name = &method_name_;
  *parameters = &parameters_;
}

CommittedEvent* SelfMethodCallCommittedEvent::Clone() const {
  return new SelfMethodCallCommittedEvent(new_shared_objects(), method_name_,
                                          parameters_);
}

void SelfMethodCallCommittedEvent::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("SELF_METHOD_CALL");

  dc->AddString("new_shared_objects");
  DumpNewSharedObjects(dc);

  dc->AddString("method_name");
  dc->AddString(method_name_);

  dc->AddString("parameters");
  dc->BeginList();
  for (const CommittedValue& parameter : parameters_) {
    parameter.Dump(dc);
  }
  dc->End();

  dc->End();
}

SelfMethodReturnCommittedEvent::SelfMethodReturnCommittedEvent(
    const unordered_set<SharedObject*>& new_shared_objects,
    const CommittedValue& return_value)
    : CommittedEvent(new_shared_objects),
      return_value_(return_value) {
}

void SelfMethodReturnCommittedEvent::GetSelfMethodReturn(
    const CommittedValue** return_value) const {
  CHECK(return_value != nullptr);
  *return_value = &return_value_;
}

CommittedEvent* SelfMethodReturnCommittedEvent::Clone() const {
  return new SelfMethodReturnCommittedEvent(new_shared_objects(),
                                            return_value_);
}

void SelfMethodReturnCommittedEvent::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("SELF_METHOD_RETURN");

  dc->AddString("new_shared_objects");
  DumpNewSharedObjects(dc);

  dc->AddString("return_value");
  return_value_.Dump(dc);

  dc->End();
}

}  // namespace engine
}  // namespace floating_temple
