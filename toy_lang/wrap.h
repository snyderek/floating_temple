// Floating Temple
// Copyright 2016 Derek S. Snyder
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

#ifndef TOY_LANG_WRAP_H_
#define TOY_LANG_WRAP_H_

#include <string>

#include "base/integral_types.h"

namespace floating_temple {

class MethodContext;
class ObjectReference;

namespace toy_lang {

ObjectReference* MakeNoneObject(MethodContext* method_context);

ObjectReference* WrapBool(MethodContext* method_context, bool b);
ObjectReference* WrapInt(MethodContext* method_context, int64 n);
ObjectReference* WrapString(MethodContext* method_context,
                            const std::string& s);

bool UnwrapBool(MethodContext* method_context,
                ObjectReference* object_reference, bool* b);
bool UnwrapInt(MethodContext* method_context, ObjectReference* object_reference,
               int64* n);
bool UnwrapString(MethodContext* method_context,
                  ObjectReference* object_reference, std::string* s);

}  // namespace toy_lang
}  // namespace floating_temple

#endif  // TOY_LANG_WRAP_H_
