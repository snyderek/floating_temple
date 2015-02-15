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

#ifndef PEER_SERIALIZATION_CONTEXT_IMPL_H_
#define PEER_SERIALIZATION_CONTEXT_IMPL_H_

#include <tr1/unordered_map>
#include <vector>

#include "base/macros.h"
#include "include/c++/serialization_context.h"

namespace floating_temple {
namespace peer {

class PeerObjectImpl;

class SerializationContextImpl : public SerializationContext {
 public:
  explicit SerializationContextImpl(std::vector<PeerObjectImpl*>* peer_objects);

  virtual int GetIndexForPeerObject(PeerObject* peer_object);

 private:
  std::vector<PeerObjectImpl*>* const peer_objects_;
  std::tr1::unordered_map<PeerObjectImpl*, int> indexes_;

  DISALLOW_COPY_AND_ASSIGN(SerializationContextImpl);
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_SERIALIZATION_CONTEXT_IMPL_H_
