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

#include "lua/global_unlock.h"

#include "base/logging.h"
#include "lua/interpreter_impl.h"

namespace floating_temple {
namespace lua {

GlobalUnlock::GlobalUnlock(InterpreterImpl* interpreter)
    : interpreter_(CHECK_NOTNULL(interpreter)) {
  interpreter->Unlock();
}

GlobalUnlock::~GlobalUnlock() {
  interpreter_->Lock();
}

}  // namespace lua
}  // namespace floating_temple
