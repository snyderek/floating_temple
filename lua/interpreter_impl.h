// Floating Temple
// Copyright 2011 Derek S. Snyder
//
// $Id: interpreter_impl.h 3410 2015-02-11 07:00:04Z dss $

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
  virtual ~InterpreterImpl();

  lua_State* GetLuaState() const;
  void SetLuaState(lua_State* lua_state);

  void BeginTransaction();
  void EndTransaction();

  Thread* GetThreadObject();
  Thread* SetThreadObject(Thread* new_thread);

  virtual LocalObject* DeserializeObject(const void* buffer,
                                         std::size_t buffer_size,
                                         DeserializationContext* context);

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
