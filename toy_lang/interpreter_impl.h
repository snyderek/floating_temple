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

#ifndef TOY_LANG_INTERPRETER_IMPL_H_
#define TOY_LANG_INTERPRETER_IMPL_H_

#include "base/macros.h"
#include "include/c++/interpreter.h"

namespace floating_temple {
namespace toy_lang {

class InterpreterImpl : public Interpreter {
 public:
  InterpreterImpl();
  ~InterpreterImpl() override;

  LocalObject* DeserializeObject(
      const void* buffer, std::size_t buffer_size,
      DeserializationContext* context) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(InterpreterImpl);
};

}  // namespace toy_lang
}  // namespace floating_temple

#endif  // TOY_LANG_INTERPRETER_IMPL_H_
