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

#include "lua/interpreter_impl.h"

#include <cstddef>

#include "base/logging.h"
#include "include/c++/thread.h"
#include "lua/table_local_object.h"
#include "third_party/lua-5.2.3/src/lauxlib.h"
#include "third_party/lua-5.2.3/src/lua.h"

using std::size_t;

namespace floating_temple {
namespace lua {

__thread lua_State* InterpreterImpl::lua_state_ = nullptr;
__thread Thread* InterpreterImpl::thread_object_ = nullptr;
__thread InterpreterImpl::LongJumpTarget* InterpreterImpl::long_jump_target_ =
    nullptr;
InterpreterImpl* InterpreterImpl::instance_ = nullptr;

InterpreterImpl::InterpreterImpl() {
  CHECK(instance_ == nullptr);
  instance_ = this;
}

InterpreterImpl::~InterpreterImpl() {
  instance_ = nullptr;
}

lua_State* InterpreterImpl::GetLuaState() {
  return PrivateGetLuaState();
}

void InterpreterImpl::BeginTransaction() {
  PrivateGetThreadObject()->BeginTransaction();
}

void InterpreterImpl::EndTransaction() {
  PrivateGetThreadObject()->EndTransaction();
}

Thread* InterpreterImpl::GetThreadObject() {
  return PrivateGetThreadObject();
}

Thread* InterpreterImpl::SetThreadObject(Thread* new_thread) {
  Thread* const old_thread = thread_object_;
  thread_object_ = new_thread;
  return old_thread;
}

InterpreterImpl::LongJumpTarget* InterpreterImpl::GetLongJumpTarget() {
  CHECK(long_jump_target_ != nullptr);
  return long_jump_target_;
}

void InterpreterImpl::SetLongJumpTarget(LongJumpTarget* target) {
  CHECK(target != nullptr);
  long_jump_target_ = target;
}

VersionedLocalObject* InterpreterImpl::DeserializeObject(
    const void* buffer, size_t buffer_size, DeserializationContext* context) {
  return TableLocalObject::Deserialize(this, buffer, buffer_size, context);
}

// static
InterpreterImpl* InterpreterImpl::instance() {
  CHECK(instance_ != nullptr);
  return instance_;
}

lua_State* InterpreterImpl::PrivateGetLuaState() {
  if (lua_state_ == nullptr) {
    lua_state_ = luaL_newstate();
    CHECK(lua_state_ != nullptr);
  }
  return lua_state_;
}

Thread* InterpreterImpl::PrivateGetThreadObject() {
  CHECK(thread_object_ != nullptr);
  return thread_object_;
}

}  // namespace lua
}  // namespace floating_temple
