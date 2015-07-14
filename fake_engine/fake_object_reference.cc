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

#include "fake_engine/fake_object_reference.h"

#include <memory>
#include <string>

#include "base/logging.h"
#include "base/string_printf.h"
#include "include/c++/local_object.h"

using std::string;

namespace floating_temple {

FakeObjectReference::FakeObjectReference(LocalObject* local_object)
    : local_object_(CHECK_NOTNULL(local_object)) {
}

FakeObjectReference::~FakeObjectReference() {
  VLOG(1) << "Deleting fake object reference " << StringPrintf("%p", this);
}

string FakeObjectReference::Dump() const {
  return StringPrintf("{ \"local_object\": \"%p\" }", local_object_.get());
}

}  // namespace floating_temple
