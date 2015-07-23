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

#include "engine/serialization_context_impl.h"

#include <unordered_map>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "engine/object_reference_impl.h"

using std::pair;
using std::unordered_map;
using std::vector;

namespace floating_temple {
namespace engine {

SerializationContextImpl::SerializationContextImpl(
    vector<ObjectReferenceImpl*>* object_references)
    : object_references_(CHECK_NOTNULL(object_references)) {
}

int SerializationContextImpl::GetIndexForObjectReference(
    ObjectReference* object_reference) {
  CHECK(object_reference != nullptr);

  ObjectReferenceImpl* const object_reference_impl =
      static_cast<ObjectReferenceImpl*>(object_reference);

  const pair<unordered_map<ObjectReferenceImpl*, int>::iterator, bool>
      insert_result = indexes_.emplace(object_reference_impl, -1);

  if (insert_result.second) {
    const int new_index = static_cast<int>(object_references_->size());
    object_references_->push_back(object_reference_impl);
    insert_result.first->second = new_index;
  }

  return insert_result.first->second;
}

}  // namespace engine
}  // namespace floating_temple
