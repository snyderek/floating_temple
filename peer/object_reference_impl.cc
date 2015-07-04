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

#include "peer/object_reference_impl.h"

#include <string>

#include "base/logging.h"
#include "base/mutex.h"
#include "base/mutex_lock.h"
#include "base/string_printf.h"
#include "peer/shared_object.h"
#include "peer/uuid_util.h"

using std::string;

namespace floating_temple {
namespace peer {

ObjectReferenceImpl::ObjectReferenceImpl(bool versioned)
    : versioned_(versioned),
      shared_object_(nullptr) {
}

ObjectReferenceImpl::~ObjectReferenceImpl() {
}

SharedObject* ObjectReferenceImpl::SetSharedObjectIfUnset(
    SharedObject* shared_object) {
  CHECK(shared_object != nullptr);

  MutexLock lock(&shared_object_mu_);

  if (shared_object_ == nullptr) {
    shared_object_ = shared_object;
  }

  return shared_object_;
}

string ObjectReferenceImpl::Dump() const {
  MutexLock lock(&shared_object_mu_);

  string shared_object_string;
  if (shared_object_ == nullptr) {
    shared_object_string = "null";
  } else {
    SStringPrintf(&shared_object_string, "\"%s\"",
                  UuidToString(shared_object_->object_id()).c_str());
  }

  return StringPrintf("{ \"shared_object\": %s }",
                      shared_object_string.c_str());
}

SharedObject* ObjectReferenceImpl::PrivateGetSharedObject() const {
  MutexLock lock(&shared_object_mu_);
  return shared_object_;
}

}  // namespace peer
}  // namespace floating_temple
