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

#include "toy_lang/zoo/function.h"

#include <string>
#include <vector>

#include "base/escape.h"
#include "base/integral_types.h"
#include "base/logging.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"

using std::string;
using std::vector;

namespace floating_temple {
namespace toy_lang {

void Function::InvokeMethod(Thread* thread,
                            ObjectReference* self_object_reference,
                            const string& method_name,
                            const vector<Value>& parameters,
                            Value* return_value) {
  CHECK(thread != nullptr);
  CHECK(return_value != nullptr);

  if (method_name == "call") {
    CHECK_EQ(parameters.size(), 1u);

    ObjectReference* const parameter_list_object =
        parameters[0].object_reference();

    Value length_value;
    if (!thread->CallMethod(parameter_list_object, "length", vector<Value>(),
                            &length_value)) {
      return;
    }

    const int64 parameter_count = length_value.int64_value();
    vector<ObjectReference*> param_objects(parameter_count);

    for (int64 i = 0; i < parameter_count; ++i) {
      vector<Value> get_at_params(1);
      get_at_params[0].set_int64_value(0, i);

      Value list_item_value;
      if (!thread->CallMethod(parameter_list_object, "get_at", get_at_params,
                              &list_item_value)) {
        return;
      }

      param_objects[i] = list_item_value.object_reference();
    }

    ObjectReference* const return_object = Call(thread, param_objects);
    if (return_object == nullptr) {
      return;
    }
    return_value->set_object_reference(0, return_object);
  } else {
    LOG(FATAL) << "Unsupported method: \"" << CEscape(method_name) << "\"";
  }
}

}  // namespace toy_lang
}  // namespace floating_temple
