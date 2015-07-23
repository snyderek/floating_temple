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
#include "include/c++/value.h"
#include "util/stl_util.h"

using std::make_pair;
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

ObjectCreationPendingEvent::ObjectCreationPendingEvent(
    ObjectReferenceImpl* prev_object_reference,
    ObjectReferenceImpl* new_object_reference,
    const shared_ptr<const LiveObject>& new_live_object)
    : PendingEvent(
          MakeSingletonSet<unordered_map<ObjectReferenceImpl*,
                                         shared_ptr<const LiveObject>>>(
              make_pair(CHECK_NOTNULL(new_object_reference), new_live_object)),
          MakeSingletonSet<unordered_set<ObjectReferenceImpl*>>(
              new_object_reference),
          prev_object_reference) {
}

BeginTransactionPendingEvent::BeginTransactionPendingEvent(
    ObjectReferenceImpl* prev_object_reference)
    : PendingEvent(
          unordered_map<ObjectReferenceImpl*, shared_ptr<const LiveObject>>(),
          unordered_set<ObjectReferenceImpl*>(),
          CHECK_NOTNULL(prev_object_reference)) {
}

EndTransactionPendingEvent::EndTransactionPendingEvent(
    ObjectReferenceImpl* prev_object_reference)
    : PendingEvent(
          unordered_map<ObjectReferenceImpl*, shared_ptr<const LiveObject>>(),
          unordered_set<ObjectReferenceImpl*>(),
          CHECK_NOTNULL(prev_object_reference)) {
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

}  // namespace engine
}  // namespace floating_temple
