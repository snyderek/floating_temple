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

#ifndef INCLUDE_CPP_PEER_H_
#define INCLUDE_CPP_PEER_H_

#include <string>

namespace floating_temple {

class UnversionedLocalObject;
class Value;

// TODO(dss): Rename the Peer and PeerImpl classes to something else, and rename
// the CanonicalPeer class to Peer.
class Peer {
 public:
  virtual ~Peer() {}

  // Calls the specified method on the specified local object and waits for the
  // method to return.
  //
  // The peer takes ownership of *local_object.
  virtual void RunProgram(UnversionedLocalObject* local_object,
                          const std::string& method_name,
                          Value* return_value,
                          bool linger) = 0;

  virtual void Stop() = 0;
};

}  // namespace floating_temple

#endif  // INCLUDE_CPP_PEER_H_
