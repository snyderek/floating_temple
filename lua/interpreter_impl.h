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

#ifndef LUA_INTERPRETER_IMPL_H_
#define LUA_INTERPRETER_IMPL_H_

#include <csetjmp>

#include "base/macros.h"
#include "include/c++/interpreter.h"

struct lua_State;

namespace floating_temple {

class Thread;

namespace lua {

class InterpreterImpl : public Interpreter {
 public:
  struct LongJumpTarget {
    std::jmp_buf env;
  };

  InterpreterImpl();
  ~InterpreterImpl() override;

  lua_State* GetLuaState();

  void BeginTransaction();
  void EndTransaction();

  Thread* GetThreadObject();
  Thread* SetThreadObject(Thread* new_thread);

  LongJumpTarget* GetLongJumpTarget();
  void SetLongJumpTarget(LongJumpTarget* target);

  VersionedLocalObject* DeserializeObject(
      const void* buffer, std::size_t buffer_size,
      DeserializationContext* context) override;

  // TODO(dss): Consider removing this method.
  static InterpreterImpl* instance();

 private:
  lua_State* PrivateGetLuaState();
  Thread* PrivateGetThreadObject();

  static __thread lua_State* lua_state_;
  static __thread Thread* thread_object_;
  static __thread LongJumpTarget* long_jump_target_;
  static InterpreterImpl* instance_;

  DISALLOW_COPY_AND_ASSIGN(InterpreterImpl);
};

}  // namespace lua
}  // namespace floating_temple

#endif  // LUA_INTERPRETER_IMPL_H_
