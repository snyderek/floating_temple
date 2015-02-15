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

#ifndef PEER_MOCK_SEQUENCE_POINT_H_
#define PEER_MOCK_SEQUENCE_POINT_H_

#include <string>

#include "base/macros.h"
#include "peer/sequence_point.h"

namespace floating_temple {
namespace peer {

class MockSequencePoint : public SequencePoint {
 public:
  MockSequencePoint();

  virtual SequencePoint* Clone() const;
  virtual std::string Dump() const;

 private:
  DISALLOW_COPY_AND_ASSIGN(MockSequencePoint);
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_MOCK_SEQUENCE_POINT_H_
