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

#ifndef ENGINE_SEQUENCE_POINT_H_
#define ENGINE_SEQUENCE_POINT_H_

#include <string>

namespace floating_temple {
namespace peer {

class SequencePoint {
 public:
  virtual ~SequencePoint() {}

  // The caller must take ownership of the returned SequencePoint instance.
  virtual SequencePoint* Clone() const = 0;

  virtual std::string Dump() const = 0;
};

}  // namespace peer
}  // namespace floating_temple

#endif  // ENGINE_SEQUENCE_POINT_H_
