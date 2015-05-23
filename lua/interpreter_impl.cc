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
#include "third_party/lua-5.2.3/src/lua.h"

using std::size_t;

namespace floating_temple {
namespace lua {

__thread Thread* InterpreterImpl::thread_object_ = nullptr;
InterpreterImpl* InterpreterImpl::instance_ = nullptr;

InterpreterImpl::InterpreterImpl()
    : lua_state_(nullptr) {
  CHECK(instance_ == nullptr);
  instance_ = this;
}

InterpreterImpl::~InterpreterImpl() {
  instance_ = nullptr;
}

lua_State* InterpreterImpl::GetLuaState() const {
  CHECK(lua_state_ != nullptr);
  return lua_state_;
}

void InterpreterImpl::SetLuaState(lua_State* lua_state) {
  CHECK(lua_state_ == nullptr);
  CHECK(lua_state != nullptr);

  lua_state_ = lua_state;
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

VersionedLocalObject* InterpreterImpl::DeserializeObject(
    const void* buffer, size_t buffer_size, DeserializationContext* context) {
  // TODO(dss): Implement this.
  return nullptr;
}

// static
InterpreterImpl* InterpreterImpl::instance() {
  CHECK(instance_ != nullptr);
  return instance_;
}

Thread* InterpreterImpl::PrivateGetThreadObject() {
  CHECK(thread_object_ != nullptr);
  return thread_object_;
}

}  // namespace lua
}  // namespace floating_temple
