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

#ifndef UTIL_STATE_VARIABLE_H_
#define UTIL_STATE_VARIABLE_H_

#include <set>
#include <utility>

#include "base/cond_var.h"
#include "base/macros.h"
#include "base/mutex.h"
#include "util/state_variable_internal_interface.h"

namespace floating_temple {

class StateVariable : private StateVariableInternalInterface {
 public:
  explicit StateVariable(unsigned starting_state);
  ~StateVariable() override;

  void AddStateTransition(unsigned old_state, unsigned new_state);

  bool MatchesStateMask(unsigned state_mask) const;
  void CheckState(unsigned expected_state_mask) const;

  unsigned WaitForState(unsigned expected_state_mask) const;
  unsigned WaitForNotState(unsigned inverse_expected_state_mask) const;
  void ChangeState(unsigned new_state);

  unsigned Mutate(void (*mutate_func)(StateVariableInternalInterface*));
  unsigned SaveOldStateAndMutate(
      void (*mutate_func)(StateVariableInternalInterface*),
      unsigned* old_state);

 private:
  bool MatchesStateMask_Locked(unsigned state_mask) const override;
  void CheckState_Locked(unsigned expected_state_mask) const override;
  void WaitForState_Locked(unsigned expected_state_mask) const override;
  void WaitForNotState_Locked(
      unsigned inverse_expected_state_mask) const override;
  void ChangeState_Locked(unsigned new_state) override;

  std::set<std::pair<unsigned, unsigned>> state_transitions_;
  unsigned current_state_;
  mutable CondVar current_state_changed_cond_;
  mutable Mutex mu_;

  DISALLOW_COPY_AND_ASSIGN(StateVariable);
};

}  // namespace floating_temple

#endif  // UTIL_STATE_VARIABLE_H_
