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

#include "lua/thread_substitution.h"

#include "base/logging.h"
#include "lua/interpreter_impl.h"

namespace floating_temple {
namespace lua {

ThreadSubstitution::ThreadSubstitution(InterpreterImpl* interpreter,
                                       Thread* new_thread)
    : interpreter_(CHECK_NOTNULL(interpreter)),
      old_thread_(interpreter->SetThreadObject(new_thread)) {
}

ThreadSubstitution::~ThreadSubstitution() {
  interpreter_->SetThreadObject(old_thread_);
}

}  // namespace lua
}  // namespace floating_temple
