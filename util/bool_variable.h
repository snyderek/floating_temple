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

#ifndef UTIL_BOOL_VARIABLE_H_
#define UTIL_BOOL_VARIABLE_H_

#include "base/macros.h"
#include "base/mutex.h"

namespace floating_temple {

// A thread-safe boolean variable.
class BoolVariable {
 public:
  explicit BoolVariable(bool initial_value);

  bool Get() const;
  void Set(bool value);

 private:
  bool value_;
  mutable Mutex mu_;

  DISALLOW_COPY_AND_ASSIGN(BoolVariable);
};

}  // namespace floating_temple

#endif  // UTIL_BOOL_VARIABLE_H_
