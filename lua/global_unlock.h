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

#ifndef LUA_GLOBAL_UNLOCK_H_
#define LUA_GLOBAL_UNLOCK_H_

#include "base/macros.h"

namespace floating_temple {
namespace lua {

class InterpreterImpl;

// Convenience class that calls InterpreterImpl::Unlock when it's constructed,
// and InterpreterImpl::Lock when it goes out of scope.
class GlobalUnlock {
 public:
  explicit GlobalUnlock(InterpreterImpl* interpreter);
  ~GlobalUnlock();

 private:
  InterpreterImpl* const interpreter_;

  DISALLOW_COPY_AND_ASSIGN(GlobalUnlock);
};

}  // namespace lua
}  // namespace floating_temple

#endif  // LUA_GLOBAL_UNLOCK_H_
