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

#include "engine/deserialization_context_impl.h"

#include <vector>

#include "base/logging.h"
#include "engine/object_reference_impl.h"

using std::vector;

namespace floating_temple {
namespace engine {

DeserializationContextImpl::DeserializationContextImpl(
    const vector<ObjectReferenceImpl*>* object_references)
    : object_references_(CHECK_NOTNULL(object_references)) {
}

ObjectReference* DeserializationContextImpl::GetObjectReferenceByIndex(
    int index) {
  CHECK_GE(index, 0);

  const vector<ObjectReferenceImpl*>::size_type vector_index =
      static_cast<vector<ObjectReferenceImpl*>::size_type>(index);
  CHECK_LT(vector_index, object_references_->size());

  return (*object_references_)[vector_index];
}

}  // namespace engine
}  // namespace floating_temple
