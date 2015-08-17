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

#include "base/macros.h"
#include "include/c++/interpreter.h"
#include "third_party/lua-5.2.3/src/lua.h"

namespace floating_temple {

class Thread;

namespace lua {

class InterpreterImpl : public Interpreter {
 public:
  InterpreterImpl();
  ~InterpreterImpl() override;

  // TODO(dss): Consider removing the GetLuaState method.
  lua_State* GetLuaState() const;
  void SetLuaState(lua_State* lua_state);

  void BeginTransaction();
  void EndTransaction();

  Thread* GetThreadObject();
  Thread* SetThreadObject(Thread* new_thread);

  VersionedLocalObject* DeserializeObject(
      const void* buffer, std::size_t buffer_size,
      DeserializationContext* context) override;

  // TODO(dss): Consider removing this method.
  static InterpreterImpl* instance();

 private:
  Thread* PrivateGetThreadObject();

  lua_State* lua_state_;
  static __thread Thread* thread_object_;
  static InterpreterImpl* instance_;

  DISALLOW_COPY_AND_ASSIGN(InterpreterImpl);
};

}  // namespace lua
}  // namespace floating_temple

#endif  // LUA_INTERPRETER_IMPL_H_
