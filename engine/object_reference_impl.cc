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

#include "engine/object_reference_impl.h"

#include <string>

#include "base/logging.h"
#include "base/string_printf.h"
#include "engine/shared_object.h"
#include "engine/uuid_util.h"
#include "util/dump_context.h"

using std::string;

namespace floating_temple {
namespace engine {

ObjectReferenceImpl::ObjectReferenceImpl(SharedObject* shared_object)
    : shared_object_(CHECK_NOTNULL(shared_object)) {
}

ObjectReferenceImpl::~ObjectReferenceImpl() {
}

string ObjectReferenceImpl::DebugString() const {
  return StringPrintf("Object reference %p (object id: %s)", this,
                      UuidToString(shared_object_->object_id()).c_str());
}

void ObjectReferenceImpl::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();
  dc->AddString("shared_object");
  dc->AddString(UuidToString(shared_object_->object_id()));
  dc->End();
}

}  // namespace engine
}  // namespace floating_temple
