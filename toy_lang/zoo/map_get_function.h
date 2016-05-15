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

#ifndef TOY_LANG_ZOO_MAP_GET_FUNCTION_H_
#define TOY_LANG_ZOO_MAP_GET_FUNCTION_H_

#include "base/macros.h"
#include "toy_lang/zoo/function.h"

namespace floating_temple {
namespace toy_lang {

class MapGetFunction : public Function {
 public:
  MapGetFunction();

  VersionedLocalObject* Clone() const override;
  void Dump(DumpContext* dc) const override;

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;
  ObjectReference* Call(
      Thread* thread,
      const std::vector<ObjectReference*>& parameters) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MapGetFunction);
};

}  // namespace toy_lang
}  // namespace floating_temple

#endif  // TOY_LANG_ZOO_MAP_GET_FUNCTION_H_
