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

#include "peer/mock_sequence_point.h"

#include <string>

#include "base/string_printf.h"

using std::string;

namespace floating_temple {
namespace peer {

MockSequencePoint::MockSequencePoint() {
}

SequencePoint* MockSequencePoint::Clone() const {
  return new MockSequencePoint();
}

string MockSequencePoint::Dump() const {
  return StringPrintf("{ \"mock_sequence_point\": \"%p\" }", this);
}

}  // namespace peer
}  // namespace floating_temple
