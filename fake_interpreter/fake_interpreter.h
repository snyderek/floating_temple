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

#ifndef FAKE_INTERPRETER_FAKE_INTERPRETER_H_
#define FAKE_INTERPRETER_FAKE_INTERPRETER_H_

#include "base/macros.h"
#include "include/c++/interpreter.h"

namespace floating_temple {

class FakeInterpreter : public Interpreter {
 public:
  FakeInterpreter();

  LocalObject* DeserializeObject(const void* buffer, std::size_t buffer_size,
                                 DeserializationContext* context) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeInterpreter);
};

}  // namespace floating_temple

#endif  // FAKE_INTERPRETER_FAKE_INTERPRETER_H_
