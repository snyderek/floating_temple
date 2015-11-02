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
#include "base/mutex.h"
#include "include/c++/thread.h"
#include "lua/table_local_object.h"
#include "lua/third_party_lua_headers.h"

using std::size_t;

namespace floating_temple {
namespace lua {

struct InterpreterImpl::PerThreadState {
  int version;
  lua_State* lua_state;
  Thread* thread_object;
  LongJumpTarget* long_jump_target;
};

__thread InterpreterImpl::PerThreadState* InterpreterImpl::per_thread_state_ =
    nullptr;
InterpreterImpl* InterpreterImpl::instance_ = nullptr;

InterpreterImpl::InterpreterImpl()
    : main_thread_lua_state_(nullptr),
      per_thread_state_version_(1) {
  CHECK(instance_ == nullptr);
  instance_ = this;
}

InterpreterImpl::~InterpreterImpl() {
  lua_close(main_thread_lua_state_);
  instance_ = nullptr;
}

void InterpreterImpl::Init() {
  CHECK(main_thread_lua_state_ == nullptr)
      << "InterpreterImpl::Init was already called.";

  main_thread_lua_state_ = luaL_newstate();
  CHECK(main_thread_lua_state_ != nullptr);

  GetPerThreadState()->lua_state = main_thread_lua_state_;
}

void InterpreterImpl::Reset() {
  CHECK(main_thread_lua_state_ != nullptr)
      << "InterpreterImpl::Init has not been called.";

  lua_close(main_thread_lua_state_);
  main_thread_lua_state_ = luaL_newstate();
  CHECK(main_thread_lua_state_ != nullptr);

  ++per_thread_state_version_;
  GetPerThreadState()->lua_state = main_thread_lua_state_;
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
  Thread** const thread_object = &GetPerThreadState()->thread_object;
  Thread* const old_thread = *thread_object;
  *thread_object = new_thread;
  return old_thread;
}

InterpreterImpl::LongJumpTarget* InterpreterImpl::GetLongJumpTarget() {
  LongJumpTarget* const target = GetPerThreadState()->long_jump_target;
  CHECK(target != nullptr);
  return target;
}

void InterpreterImpl::SetLongJumpTarget(LongJumpTarget* target) {
  CHECK(target != nullptr);
  GetPerThreadState()->long_jump_target = target;
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

InterpreterImpl::PerThreadState* InterpreterImpl::GetPerThreadState() {
  CHECK(main_thread_lua_state_ != nullptr)
      << "InterpreterImpl::Init has not been called.";

  if (per_thread_state_ == nullptr ||
      per_thread_state_->version != per_thread_state_version_) {
    if (per_thread_state_ == nullptr) {
      per_thread_state_ = new PerThreadState();
    }

    per_thread_state_->version = per_thread_state_version_;
    per_thread_state_->lua_state = nullptr;
    per_thread_state_->thread_object = nullptr;
    per_thread_state_->long_jump_target = nullptr;
  }

  return per_thread_state_;
}

lua_State* InterpreterImpl::PrivateGetLuaState() {
  lua_State** const lua_state = &GetPerThreadState()->lua_state;
  if (*lua_state == nullptr) {
    *lua_state = lua_newthread(main_thread_lua_state_);
    CHECK(*lua_state != nullptr);
  }
  return *lua_state;
}

Thread* InterpreterImpl::PrivateGetThreadObject() {
  Thread* const thread_object = GetPerThreadState()->thread_object;
  CHECK(thread_object != nullptr);
  return thread_object;
}

}  // namespace lua
}  // namespace floating_temple
