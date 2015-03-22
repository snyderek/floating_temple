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

#include "peer/committed_event.h"

#include <string>
#include <unordered_set>
#include <vector>

#include "base/escape.h"
#include "base/logging.h"
#include "base/string_printf.h"
#include "peer/committed_value.h"
#include "peer/const_live_object_ptr.h"
#include "peer/live_object.h"
#include "peer/shared_object.h"
#include "peer/uuid_util.h"
#include "util/stl_util.h"

using std::string;
using std::unordered_set;
using std::vector;

namespace floating_temple {
namespace peer {

CommittedEvent::CommittedEvent(
    const unordered_set<SharedObject*>& new_shared_objects)
    : new_shared_objects_(new_shared_objects) {
  CHECK(new_shared_objects.find(nullptr) == new_shared_objects.end());
}

void CommittedEvent::GetObjectCreation(ConstLiveObjectPtr* live_object) const {
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

string CommittedEvent::DumpNewSharedObjects() const {
  if (new_shared_objects_.empty()) {
    return "[]";
  }

  string s = "[";
  for (unordered_set<SharedObject*>::const_iterator it =
           new_shared_objects_.begin();
       it != new_shared_objects_.end(); ++it) {
    if (it != new_shared_objects_.begin()) {
      s += ',';
    }

    StringAppendF(&s, " \"%s\"", UuidToString((*it)->object_id()).c_str());
  }
  s += " ]";

  return s;
}

ObjectCreationCommittedEvent::ObjectCreationCommittedEvent(
    const ConstLiveObjectPtr& live_object)
    : CommittedEvent(unordered_set<SharedObject*>()),
      live_object_(live_object) {
  CHECK(live_object.get() != nullptr);
}

void ObjectCreationCommittedEvent::GetObjectCreation(
    ConstLiveObjectPtr* live_object) const {
  CHECK(live_object != nullptr);
  *live_object = live_object_;
}

CommittedEvent* ObjectCreationCommittedEvent::Clone() const {
  return new ObjectCreationCommittedEvent(live_object_);
}

string ObjectCreationCommittedEvent::Dump() const {
  return StringPrintf(
      "{ \"type\": \"OBJECT_CREATION\", \"new_shared_objects\": %s, "
      "\"live_object\": %s }",
      DumpNewSharedObjects().c_str(), live_object_->Dump().c_str());
}

SubObjectCreationCommittedEvent::SubObjectCreationCommittedEvent(
    SharedObject* new_shared_object)
    : CommittedEvent(
          MakeSingletonSet<unordered_set<SharedObject*>>(
                CHECK_NOTNULL(new_shared_object))) {
}

CommittedEvent* SubObjectCreationCommittedEvent::Clone() const {
  CHECK_EQ(new_shared_objects().size(), 1u);
  return new SubObjectCreationCommittedEvent(GetNewSharedObject());
}

string SubObjectCreationCommittedEvent::Dump() const {
  CHECK_EQ(new_shared_objects().size(), 1u);
  return StringPrintf(
      "{ \"type\": \"SUB_OBJECT_CREATION\", \"new_shared_object\": \"%s\" }",
      UuidToString(GetNewSharedObject()->object_id()).c_str());
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

string BeginTransactionCommittedEvent::Dump() const {
  return StringPrintf(
      "{ \"type\": \"BEGIN_TRANSACTION\", \"new_shared_objects\": %s }",
      DumpNewSharedObjects().c_str());
}

EndTransactionCommittedEvent::EndTransactionCommittedEvent()
    : CommittedEvent(unordered_set<SharedObject*>()) {
}

CommittedEvent* EndTransactionCommittedEvent::Clone() const {
  return new EndTransactionCommittedEvent();
}

string EndTransactionCommittedEvent::Dump() const {
  return StringPrintf(
      "{ \"type\": \"END_TRANSACTION\", \"new_shared_objects\": %s }",
      DumpNewSharedObjects().c_str());
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

string MethodCallCommittedEvent::Dump() const {
  string caller_string;
  if (caller_== nullptr) {
    caller_string = "null";
  } else {
    SStringPrintf(&caller_string, "\"%s\"",
                  UuidToString(caller_->object_id()).c_str());
  }

  string parameters_string;
  if (parameters_.empty()) {
    parameters_string = "[]";
  } else {
    parameters_string = "[";
    for (vector<CommittedValue>::const_iterator it = parameters_.begin();
         it != parameters_.end(); ++it) {
      if (it != parameters_.begin()) {
        parameters_string += ",";
      }

      StringAppendF(&parameters_string, " %s", it->Dump().c_str());
    }
    parameters_string += " ]";
  }

  return StringPrintf(
      "{ \"type\": \"METHOD_CALL\", \"new_shared_objects\": %s, "
      "\"caller\": %s, \"method_name\": \"%s\", \"parameters\": %s }",
      DumpNewSharedObjects().c_str(), caller_string.c_str(),
      CEscape(method_name_).c_str(), parameters_string.c_str());
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

string MethodReturnCommittedEvent::Dump() const {
  string caller_string;
  if (caller_== nullptr) {
    caller_string = "null";
  } else {
    SStringPrintf(&caller_string, "\"%s\"",
                  UuidToString(caller_->object_id()).c_str());
  }

  return StringPrintf(
      "{ \"type\": \"METHOD_RETURN\", \"new_shared_objects\": %s, "
      "\"caller\": %s, \"return_value\": %s }",
      DumpNewSharedObjects().c_str(), caller_string.c_str(),
      return_value_.Dump().c_str());
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

string SubMethodCallCommittedEvent::Dump() const {
  string parameters_string;
  if (parameters_.empty()) {
    parameters_string = "[]";
  } else {
    parameters_string = "[";
    for (vector<CommittedValue>::const_iterator it = parameters_.begin();
         it != parameters_.end(); ++it) {
      if (it != parameters_.begin()) {
        parameters_string += ",";
      }

      StringAppendF(&parameters_string, " %s", it->Dump().c_str());
    }
    parameters_string += " ]";
  }

  return StringPrintf(
      "{ \"type\": \"SUB_METHOD_CALL\", \"new_shared_objects\": %s, "
      "\"callee\": \"%s\", \"method_name\": \"%s\", \"parameters\": %s }",
      DumpNewSharedObjects().c_str(),
      UuidToString(callee_->object_id()).c_str(), CEscape(method_name_).c_str(),
      parameters_string.c_str());
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

string SubMethodReturnCommittedEvent::Dump() const {
  return StringPrintf(
      "{ \"type\": \"SUB_METHOD_RETURN\", \"new_shared_objects\": %s, "
      "\"callee\": \"%s\", \"return_value\": %s }",
      DumpNewSharedObjects().c_str(),
      UuidToString(callee_->object_id()).c_str(), return_value_.Dump().c_str());
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

string SelfMethodCallCommittedEvent::Dump() const {
  string parameters_string;
  if (parameters_.empty()) {
    parameters_string = "[]";
  } else {
    parameters_string = "[";
    for (vector<CommittedValue>::const_iterator it = parameters_.begin();
         it != parameters_.end(); ++it) {
      if (it != parameters_.begin()) {
        parameters_string += ",";
      }

      StringAppendF(&parameters_string, " %s", it->Dump().c_str());
    }
    parameters_string += " ]";
  }

  return StringPrintf(
      "{ \"type\": \"SELF_METHOD_CALL\", \"new_shared_objects\": %s, "
      "\"method_name\": \"%s\", \"parameters\": %s }",
      DumpNewSharedObjects().c_str(), CEscape(method_name_).c_str(),
      parameters_string.c_str());
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

string SelfMethodReturnCommittedEvent::Dump() const {
  return StringPrintf(
      "{ \"type\": \"SELF_METHOD_RETURN\", \"new_shared_objects\": %s, "
      "\"return_value\": %s }",
      DumpNewSharedObjects().c_str(), return_value_.Dump().c_str());
}

}  // namespace peer
}  // namespace floating_temple
