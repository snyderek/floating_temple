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

#include "engine/pending_event.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "engine/live_object.h"
#include "engine/object_reference_impl.h"
#include "include/c++/value.h"
#include "util/dump_context.h"
#include "util/stl_util.h"

using std::pair;
using std::shared_ptr;
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

namespace floating_temple {
namespace engine {

PendingEvent::PendingEvent(
    const unordered_map<ObjectReferenceImpl*, shared_ptr<const LiveObject>>&
        live_objects,
    const unordered_set<ObjectReferenceImpl*>& new_object_references,
    ObjectReferenceImpl* prev_object_reference)
    : live_objects_(live_objects),
      new_object_references_(new_object_references),
      prev_object_reference_(prev_object_reference) {
  // Check that new_object_references is a subset of keys(live_objects).
  for (ObjectReferenceImpl* const object_reference : new_object_references) {
    CHECK(live_objects.find(object_reference) != live_objects.end());
  }
}

void PendingEvent::GetMethodCall(ObjectReferenceImpl** next_object_reference,
                                 const string** method_name,
                                 const vector<Value>** parameters) const {
  LOG(FATAL) << "Invalid call to GetMethodCall (type == "
             << static_cast<int>(this->type()) << ")";
}

void PendingEvent::GetMethodReturn(ObjectReferenceImpl** next_object_reference,
                                   const Value** return_value) const {
  LOG(FATAL) << "Invalid call to GetMethodReturn (type == "
             << static_cast<int>(this->type()) << ")";
}

#define CHECK_EVENT_TYPE(event_type) \
  case PendingEvent::event_type: \
    do { \
      return #event_type; \
    } while (false)

// static
string PendingEvent::GetTypeString(Type event_type) {
  switch (event_type) {
    CHECK_EVENT_TYPE(BEGIN_TRANSACTION);
    CHECK_EVENT_TYPE(END_TRANSACTION);
    CHECK_EVENT_TYPE(METHOD_CALL);
    CHECK_EVENT_TYPE(METHOD_RETURN);

    default:
      LOG(FATAL) << "Invalid event type: " << static_cast<int>(event_type);
  }
}

#undef CHECK_EVENT_TYPE

void PendingEvent::DumpAffectedObjects(DumpContext* dc) const {
  dc->AddString("live_objects");
  dc->BeginList();
  for (const pair<ObjectReferenceImpl*, shared_ptr<const LiveObject>>&
           live_object_pair : live_objects_) {
    dc->BeginList();
    live_object_pair.first->Dump(dc);
    live_object_pair.second->Dump(dc);
    dc->End();
  }
  dc->End();

  dc->AddString("new_object_references");
  dc->BeginList();
  for (const ObjectReferenceImpl* const object_reference :
           new_object_references_) {
    object_reference->Dump(dc);
  }
  dc->End();

  dc->AddString("prev_object_reference");
  prev_object_reference_->Dump(dc);
}

BeginTransactionPendingEvent::BeginTransactionPendingEvent(
    ObjectReferenceImpl* prev_object_reference)
    : PendingEvent(
          unordered_map<ObjectReferenceImpl*, shared_ptr<const LiveObject>>(),
          unordered_set<ObjectReferenceImpl*>(),
          CHECK_NOTNULL(prev_object_reference)) {
}

void BeginTransactionPendingEvent::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("BEGIN_TRANSACTION");

  DumpAffectedObjects(dc);

  dc->End();
}

EndTransactionPendingEvent::EndTransactionPendingEvent(
    ObjectReferenceImpl* prev_object_reference)
    : PendingEvent(
          unordered_map<ObjectReferenceImpl*, shared_ptr<const LiveObject>>(),
          unordered_set<ObjectReferenceImpl*>(),
          CHECK_NOTNULL(prev_object_reference)) {
}

void EndTransactionPendingEvent::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("END_TRANSACTION");

  DumpAffectedObjects(dc);

  dc->End();
}

MethodCallPendingEvent::MethodCallPendingEvent(
    const unordered_map<ObjectReferenceImpl*, shared_ptr<const LiveObject>>&
        live_objects,
    const unordered_set<ObjectReferenceImpl*>& new_object_references,
    ObjectReferenceImpl* prev_object_reference,
    ObjectReferenceImpl* next_object_reference,
    const string& method_name,
    const vector<Value>& parameters)
    : PendingEvent(live_objects, new_object_references, prev_object_reference),
      next_object_reference_(next_object_reference),
      method_name_(method_name),
      parameters_(parameters) {
  CHECK(!method_name.empty());
}

void MethodCallPendingEvent::GetMethodCall(
    ObjectReferenceImpl** next_object_reference, const string** method_name,
    const vector<Value>** parameters) const {
  CHECK(next_object_reference != nullptr);
  CHECK(method_name != nullptr);
  CHECK(parameters != nullptr);

  *next_object_reference = next_object_reference_;
  *method_name = &method_name_;
  *parameters = &parameters_;
}

void MethodCallPendingEvent::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("METHOD_CALL");

  dc->AddString("next_object_reference");
  next_object_reference_->Dump(dc);

  dc->AddString("method_name");
  dc->AddString(method_name_);

  dc->AddString("parameters");
  dc->BeginList();
  for (const Value& value : parameters_) {
    value.Dump(dc);
  }
  dc->End();

  DumpAffectedObjects(dc);

  dc->End();
}

MethodReturnPendingEvent::MethodReturnPendingEvent(
    const unordered_map<ObjectReferenceImpl*, shared_ptr<const LiveObject>>&
        live_objects,
    const unordered_set<ObjectReferenceImpl*>& new_object_references,
    ObjectReferenceImpl* prev_object_reference,
    ObjectReferenceImpl* next_object_reference,
    const Value& return_value)
    : PendingEvent(live_objects, new_object_references, prev_object_reference),
      next_object_reference_(next_object_reference),
      return_value_(return_value) {
}

void MethodReturnPendingEvent::GetMethodReturn(
    ObjectReferenceImpl** next_object_reference,
    const Value** return_value) const {
  CHECK(next_object_reference != nullptr);
  CHECK(return_value != nullptr);

  *next_object_reference = next_object_reference_;
  *return_value = &return_value_;
}

void MethodReturnPendingEvent::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("METHOD_RETURN");

  dc->AddString("next_object_reference");
  next_object_reference_->Dump(dc);

  dc->AddString("return_value");
  return_value_.Dump(dc);

  DumpAffectedObjects(dc);

  dc->End();
}

}  // namespace engine
}  // namespace floating_temple
