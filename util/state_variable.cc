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

#include "util/state_variable.h"

#include <cstddef>
#include <set>
#include <utility>

#include "base/cond_var.h"
#include "base/logging.h"
#include "base/mutex.h"
#include "base/mutex_lock.h"
#include "util/math_util.h"

using std::make_pair;

namespace floating_temple {

StateVariable::StateVariable(unsigned starting_state)
    : current_state_(starting_state) {
  CHECK(IsPowerOfTwo(starting_state)) << "starting_state == " << starting_state;
}

StateVariable::~StateVariable() {
}

void StateVariable::AddStateTransition(unsigned old_state, unsigned new_state) {
  CHECK(IsPowerOfTwo(old_state)) << "old_state == " << old_state;
  CHECK(IsPowerOfTwo(new_state)) << "new_state == " << new_state;
  CHECK_NE(old_state, new_state);

  MutexLock lock(&mu_);
  state_transitions_.insert(make_pair(old_state, new_state));
}

bool StateVariable::MatchesStateMask(unsigned state_mask) const {
  MutexLock lock(&mu_);
  return MatchesStateMask_Locked(state_mask);
}

void StateVariable::CheckState(unsigned expected_state_mask) const {
  MutexLock lock(&mu_);
  CheckState_Locked(expected_state_mask);
}

unsigned StateVariable::WaitForState(unsigned expected_state_mask) const {
  MutexLock lock(&mu_);
  WaitForState_Locked(expected_state_mask);
  return current_state_;
}

unsigned StateVariable::WaitForNotState(
    unsigned inverse_expected_state_mask) const {
  MutexLock lock(&mu_);
  WaitForNotState_Locked(inverse_expected_state_mask);
  return current_state_;
}

void StateVariable::ChangeState(unsigned new_state) {
  MutexLock lock(&mu_);
  ChangeState_Locked(new_state);
}

unsigned StateVariable::Mutate(
    void (*mutate_func)(StateVariableInternalInterface*)) {
  MutexLock lock(&mu_);
  (*mutate_func)(this);
  return current_state_;
}

unsigned StateVariable::SaveOldStateAndMutate(
    void (*mutate_func)(StateVariableInternalInterface*), unsigned* old_state) {
  CHECK(old_state != NULL);

  MutexLock lock(&mu_);
  *old_state = current_state_;
  (*mutate_func)(this);
  return current_state_;
}

bool StateVariable::MatchesStateMask_Locked(unsigned state_mask) const {
  CHECK_NE(state_mask, 0);
  return (current_state_ & state_mask) != 0;
}

void StateVariable::CheckState_Locked(unsigned expected_state_mask) const {
  CHECK(MatchesStateMask_Locked(expected_state_mask))
      << "expected_state_mask == " << expected_state_mask
      << ", current_state_ == " << current_state_;
}

void StateVariable::WaitForState_Locked(unsigned expected_state_mask) const {
  while (!MatchesStateMask_Locked(expected_state_mask)) {
    current_state_changed_cond_.Wait(&mu_);
  }
}

void StateVariable::WaitForNotState_Locked(
    unsigned inverse_expected_state_mask) const {
  while (MatchesStateMask_Locked(inverse_expected_state_mask)) {
    current_state_changed_cond_.Wait(&mu_);
  }
}

void StateVariable::ChangeState_Locked(unsigned new_state) {
  CHECK(state_transitions_.find(make_pair(current_state_, new_state)) !=
        state_transitions_.end())
      << "current_state_ == " << current_state_
      << ", new_state == " << new_state;

  current_state_ = new_state;
  current_state_changed_cond_.Broadcast();
}

}  // namespace floating_temple
