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

#ifndef TOY_LANG_ZOO_LOCAL_OBJECT_IMPL_H_
#define TOY_LANG_ZOO_LOCAL_OBJECT_IMPL_H_

#include <cstddef>

#include "include/c++/local_object.h"

namespace floating_temple {

class DeserializationContext;
class SerializationContext;

namespace toy_lang {

class ObjectProto;

class LocalObjectImpl : public LocalObject {
 public:
  std::size_t Serialize(void* buffer, std::size_t buffer_size,
                        SerializationContext* context) const override;

  static LocalObjectImpl* Deserialize(const void* buffer,
                                      std::size_t buffer_size,
                                      DeserializationContext* context);

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const = 0;
};

}  // namespace toy_lang
}  // namespace floating_temple

#endif  // TOY_LANG_ZOO_LOCAL_OBJECT_IMPL_H_
