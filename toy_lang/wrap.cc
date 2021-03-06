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

#include "toy_lang/wrap.h"

#include <string>
#include <vector>

#include "base/integral_types.h"
#include "base/logging.h"
#include "include/c++/method_context.h"
#include "include/c++/value.h"
#include "toy_lang/zoo/bool_object.h"
#include "toy_lang/zoo/int_object.h"
#include "toy_lang/zoo/none_object.h"
#include "toy_lang/zoo/string_object.h"

using std::string;
using std::vector;

namespace floating_temple {
namespace toy_lang {

ObjectReference* MakeNoneObject(MethodContext* method_context) {
  CHECK(method_context != nullptr);
  // TODO(dss): Consider having just a single instance of the "none" object.
  return method_context->CreateObject(new NoneObject(), "");
}

ObjectReference* WrapBool(MethodContext* method_context, bool b) {
  CHECK(method_context != nullptr);
  return method_context->CreateObject(new BoolObject(b), "");
}

ObjectReference* WrapInt(MethodContext* method_context, int64 n) {
  CHECK(method_context != nullptr);
  return method_context->CreateObject(new IntObject(n), "");
}

ObjectReference* WrapString(MethodContext* method_context, const string& s) {
  CHECK(method_context != nullptr);
  return method_context->CreateObject(new StringObject(s), "");
}

bool UnwrapBool(MethodContext* method_context,
                ObjectReference* object_reference, bool* b) {
  CHECK(method_context != nullptr);
  CHECK(b != nullptr);

  Value value;
  if (!method_context->CallMethod(object_reference, "get_bool", vector<Value>(),
                                  &value)) {
    return false;
  }

  *b = value.bool_value();
  return true;
}

bool UnwrapInt(MethodContext* method_context, ObjectReference* object_reference,
               int64* n) {
  CHECK(method_context != nullptr);
  CHECK(n != nullptr);

  Value value;
  if (!method_context->CallMethod(object_reference, "get_int", vector<Value>(),
                                  &value)) {
    return false;
  }

  *n = value.int64_value();
  return true;
}

bool UnwrapString(MethodContext* method_context,
                  ObjectReference* object_reference, string* s) {
  CHECK(method_context != nullptr);
  CHECK(s != nullptr);

  Value value;
  if (!method_context->CallMethod(object_reference, "get_string",
                                  vector<Value>(), &value)) {
    return false;
  }

  *s = value.string_value();
  return true;
}

}  // namespace toy_lang
}  // namespace floating_temple
