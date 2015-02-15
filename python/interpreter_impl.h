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

#ifndef PYTHON_INTERPRETER_IMPL_H_
#define PYTHON_INTERPRETER_IMPL_H_

#include "base/macros.h"
#include "include/c++/interpreter.h"

namespace floating_temple {

class Thread;

namespace python {

class InterpreterImpl : public Interpreter {
 public:
  InterpreterImpl();
  virtual ~InterpreterImpl();

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

  static __thread Thread* thread_object_;
  static InterpreterImpl* instance_;

  DISALLOW_COPY_AND_ASSIGN(InterpreterImpl);
};

}  // namespace python
}  // namespace floating_temple

#endif  // PYTHON_INTERPRETER_IMPL_H_
