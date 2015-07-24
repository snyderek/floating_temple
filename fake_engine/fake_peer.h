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

#ifndef FAKE_ENGINE_FAKE_PEER_H_
#define FAKE_ENGINE_FAKE_PEER_H_

#include <string>

#include "base/macros.h"
#include "include/c++/peer.h"

namespace floating_temple {

class FakePeer : public Peer {
 public:
  FakePeer();

  void RunProgram(UnversionedLocalObject* local_object,
                  const std::string& method_name,
                  Value* return_value,
                  bool linger) override;
  void Stop() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(FakePeer);
};

}  // namespace floating_temple

#endif  // FAKE_ENGINE_FAKE_PEER_H_