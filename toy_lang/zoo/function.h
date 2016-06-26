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

#ifndef TOY_LANG_ZOO_FUNCTION_H_
#define TOY_LANG_ZOO_FUNCTION_H_

#include <vector>

#include "toy_lang/zoo/local_object_impl.h"

namespace floating_temple {

class MethodContext;
class ObjectReference;

namespace toy_lang {

class Function : public LocalObjectImpl {
 public:
  void InvokeMethod(MethodContext* method_context,
                    ObjectReference* self_object_reference,
                    const std::string& method_name,
                    const std::vector<Value>& parameters,
                    Value* return_value) override;

 protected:
  virtual ObjectReference* Call(
      MethodContext* method_context,
      const std::vector<ObjectReference*>& parameters) const = 0;
};

}  // namespace toy_lang
}  // namespace floating_temple

#endif  // TOY_LANG_ZOO_FUNCTION_H_
