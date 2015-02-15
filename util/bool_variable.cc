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

#include "util/bool_variable.h"

#include "base/mutex.h"
#include "base/mutex_lock.h"

namespace floating_temple {

BoolVariable::BoolVariable(bool initial_value)
    : value_(initial_value) {
}

bool BoolVariable::Get() const {
  MutexLock lock(&mu_);
  return value_;
}

void BoolVariable::Set(bool value) {
  MutexLock lock(&mu_);
  value_ = value;
}

}  // namespace floating_temple
