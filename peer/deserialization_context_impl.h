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

#ifndef PEER_DESERIALIZATION_CONTEXT_IMPL_H_
#define PEER_DESERIALIZATION_CONTEXT_IMPL_H_

#include <vector>

#include "base/macros.h"
#include "include/c++/deserialization_context.h"

namespace floating_temple {
namespace peer {

class PeerObjectImpl;

class DeserializationContextImpl : public DeserializationContext {
 public:
  explicit DeserializationContextImpl(
      const std::vector<PeerObjectImpl*>* peer_objects);

  virtual PeerObject* GetPeerObjectByIndex(int index);

 private:
  const std::vector<PeerObjectImpl*>* const peer_objects_;

  DISALLOW_COPY_AND_ASSIGN(DeserializationContextImpl);
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_DESERIALIZATION_CONTEXT_IMPL_H_
