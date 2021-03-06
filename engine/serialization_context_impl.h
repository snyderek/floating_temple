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

#ifndef ENGINE_SERIALIZATION_CONTEXT_IMPL_H_
#define ENGINE_SERIALIZATION_CONTEXT_IMPL_H_

#include <unordered_map>
#include <vector>

#include "base/macros.h"
#include "include/c++/serialization_context.h"

namespace floating_temple {
namespace engine {

class ObjectReferenceImpl;

class SerializationContextImpl : public SerializationContext {
 public:
  explicit SerializationContextImpl(
      std::vector<ObjectReferenceImpl*>* object_references);

  int GetIndexForObjectReference(ObjectReference* object_reference) override;

 private:
  std::vector<ObjectReferenceImpl*>* const object_references_;
  std::unordered_map<ObjectReferenceImpl*, int> indexes_;

  DISALLOW_COPY_AND_ASSIGN(SerializationContextImpl);
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_SERIALIZATION_CONTEXT_IMPL_H_
