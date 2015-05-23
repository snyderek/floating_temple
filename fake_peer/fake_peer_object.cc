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

#include "fake_peer/fake_peer_object.h"

#include <memory>
#include <string>

#include "base/logging.h"
#include "base/string_printf.h"
#include "include/c++/versioned_local_object.h"

using std::string;

namespace floating_temple {

FakePeerObject::FakePeerObject(VersionedLocalObject* local_object)
    : local_object_(CHECK_NOTNULL(local_object)) {
}

FakePeerObject::~FakePeerObject() {
  VLOG(1) << "Deleting fake peer object " << StringPrintf("%p", this);
}

string FakePeerObject::Dump() const {
  return StringPrintf("{ \"local_object\": \"%p\" }", local_object_.get());
}

}  // namespace floating_temple
