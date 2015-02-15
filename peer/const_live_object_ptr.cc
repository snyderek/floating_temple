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

#include "peer/const_live_object_ptr.h"

#include <cstddef>

#include "base/logging.h"
#include "peer/live_object.h"
#include "peer/live_object_ptr.h"

namespace floating_temple {
namespace peer {

ConstLiveObjectPtr::ConstLiveObjectPtr(LiveObject* live_object)
    : live_object_(live_object) {
  IncrementRefCount();
}

ConstLiveObjectPtr::ConstLiveObjectPtr(const ConstLiveObjectPtr& other)
    : live_object_(other.live_object_) {
  IncrementRefCount();
}

ConstLiveObjectPtr::ConstLiveObjectPtr(const LiveObjectPtr& other)
    : live_object_(other.live_object_) {
  IncrementRefCount();
}

ConstLiveObjectPtr::~ConstLiveObjectPtr() {
  DecrementRefCount();
}

void ConstLiveObjectPtr::reset(LiveObject* live_object) {
  Assign(live_object);
}

ConstLiveObjectPtr& ConstLiveObjectPtr::operator=(
    const ConstLiveObjectPtr& other) {
  Assign(other.live_object_);
  return *this;
}

ConstLiveObjectPtr& ConstLiveObjectPtr::operator=(const LiveObjectPtr& other) {
  Assign(other.live_object_);
  return *this;
}

const LiveObject& ConstLiveObjectPtr::operator*() const {
  CHECK(live_object_ != NULL);
  return *live_object_;
}

const LiveObject* ConstLiveObjectPtr::operator->() const {
  CHECK(live_object_ != NULL);
  return live_object_;
}

void ConstLiveObjectPtr::Assign(LiveObject* live_object) {
  if (live_object_ != live_object) {
    DecrementRefCount();
    live_object_ = live_object;
    IncrementRefCount();
  }
}

void ConstLiveObjectPtr::IncrementRefCount() {
  if (live_object_ != NULL) {
    live_object_->IncrementRefCount();
  }
}

void ConstLiveObjectPtr::DecrementRefCount() {
  if (live_object_ != NULL) {
    if (live_object_->DecrementRefCount()) {
      delete live_object_;
    }
  }
}

}  // namespace peer
}  // namespace floating_temple
