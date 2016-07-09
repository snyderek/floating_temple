// Floating Temple
// Copyright 2016 Derek S. Snyder
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

#include "engine/recording_method_context.h"

#include <memory>
#include <string>
#include <vector>

#include "base/logging.h"
#include "engine/live_object.h"
#include "engine/object_reference_impl.h"
#include "engine/recording_thread_internal_interface.h"

using std::shared_ptr;
using std::string;
using std::vector;

namespace floating_temple {
namespace engine {

RecordingMethodContext::RecordingMethodContext(
    RecordingThreadInternalInterface* recording_thread,
    ObjectReferenceImpl* current_object_reference,
    const shared_ptr<LiveObject>& current_live_object)
    : recording_thread_(CHECK_NOTNULL(recording_thread)),
      current_object_reference_(current_object_reference),
      current_live_object_(current_live_object) {
  CHECK(current_live_object.get() != nullptr);
}

RecordingMethodContext::~RecordingMethodContext() {
}

bool RecordingMethodContext::BeginTransaction() {
  return recording_thread_->BeginTransaction(current_object_reference_,
                                             current_live_object_);
}

bool RecordingMethodContext::EndTransaction() {
  return recording_thread_->EndTransaction(current_object_reference_,
                                           current_live_object_);
}

ObjectReference* RecordingMethodContext::CreateObject(
    LocalObject* initial_version, const string& name) {
  return recording_thread_->CreateObject(initial_version, name);
}

bool RecordingMethodContext::CallMethod(ObjectReference* object_reference,
                                        const string& method_name,
                                        const vector<Value>& parameters,
                                        Value* return_value) {
  return recording_thread_->CallMethod(
      current_object_reference_, current_live_object_,
      static_cast<ObjectReferenceImpl*>(object_reference), method_name,
      parameters, return_value);
}

bool RecordingMethodContext::ObjectsAreIdentical(
    const ObjectReference* a, const ObjectReference* b) const {
  return recording_thread_->ObjectsAreIdentical(
      static_cast<const ObjectReferenceImpl*>(a),
      static_cast<const ObjectReferenceImpl*>(b));
}

}  // namespace engine
}  // namespace floating_temple
