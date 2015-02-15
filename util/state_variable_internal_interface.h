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

#ifndef UTIL_STATE_VARIABLE_INTERNAL_INTERFACE_H_
#define UTIL_STATE_VARIABLE_INTERNAL_INTERFACE_H_

namespace floating_temple {

class StateVariableInternalInterface {
 public:
  virtual ~StateVariableInternalInterface() {}

  // TODO(dss): Remove the suffix "_Locked" from these method names.
  virtual bool MatchesStateMask_Locked(unsigned state_mask) const = 0;
  virtual void CheckState_Locked(unsigned expected_state_mask) const = 0;

  virtual void WaitForState_Locked(unsigned expected_state_mask) const = 0;
  virtual void WaitForNotState_Locked(
      unsigned inverse_expected_state_mask) const = 0;

  virtual void ChangeState_Locked(unsigned new_state) = 0;
};

}  // namespace floating_temple

#endif  // UTIL_STATE_VARIABLE_INTERNAL_INTERFACE_H_
