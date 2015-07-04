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

#include "toy_lang/symbol_table.h"

#include <string>
#include <vector>

#include "base/escape.h"
#include "base/logging.h"
#include "include/c++/object_reference.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"

using std::string;
using std::vector;

namespace floating_temple {
namespace toy_lang {

bool EnterScope(ObjectReference* symbol_table_object, Thread* thread) {
  CHECK(thread != nullptr);

  Value dummy;
  return thread->CallMethod(symbol_table_object, "enter_scope", vector<Value>(),
                            &dummy);
}

bool LeaveScope(ObjectReference* symbol_table_object, Thread* thread) {
  CHECK(thread != nullptr);

  Value dummy;
  return thread->CallMethod(symbol_table_object, "leave_scope", vector<Value>(),
                            &dummy);
}

bool IsVariableSet(ObjectReference* symbol_table_object, Thread* thread,
                   const string& name, bool* is_set) {
  CHECK(thread != nullptr);
  CHECK(is_set != nullptr);

  VLOG(1) << "Symbol table: Is set \"" << CEscape(name) << "\"";

  vector<Value> params(1);
  params[0].set_string_value(0, name);

  Value is_set_value;
  if (!thread->CallMethod(symbol_table_object, "is_set", params,
                          &is_set_value)) {
    return false;
  }

  *is_set = is_set_value.bool_value();
  return true;
}

bool GetVariable(ObjectReference* symbol_table_object, Thread* thread,
                 const string& name, ObjectReference** object) {
  CHECK(thread != nullptr);
  CHECK(object != nullptr);

  VLOG(1) << "Symbol table: Get \"" << CEscape(name) << "\"";

  vector<Value> params(1);
  params[0].set_string_value(0, name);

  Value object_value;
  if (!thread->CallMethod(symbol_table_object, "get", params, &object_value)) {
    return false;
  }

  *object = object_value.object_reference();
  return true;
}

bool SetVariable(ObjectReference* symbol_table_object, Thread* thread,
                 const string& name, ObjectReference* object) {
  CHECK(thread != nullptr);

  VLOG(1) << "Symbol table: Set \"" << CEscape(name) << "\"";

  vector<Value> params(2);
  params[0].set_string_value(0, name);
  params[1].set_object_reference(0, object);

  Value dummy;
  if (!thread->CallMethod(symbol_table_object, "set", params, &dummy)) {
    return false;
  }

  return true;
}

}  // namespace toy_lang
}  // namespace floating_temple
