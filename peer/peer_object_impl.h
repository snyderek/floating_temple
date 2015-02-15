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

#ifndef PEER_PEER_OBJECT_IMPL_H_
#define PEER_PEER_OBJECT_IMPL_H_

#include <string>

#include "base/macros.h"
#include "base/mutex.h"
#include "include/c++/peer_object.h"

namespace floating_temple {
namespace peer {

class SharedObject;

class PeerObjectImpl : public PeerObject {
 public:
  PeerObjectImpl();
  virtual ~PeerObjectImpl();

  const SharedObject* shared_object() const { return PrivateGetSharedObject(); }
  SharedObject* shared_object() { return PrivateGetSharedObject(); }

  SharedObject* SetSharedObjectIfUnset(SharedObject* shared_object);

  virtual std::string Dump() const;

 private:
  SharedObject* PrivateGetSharedObject() const;

  SharedObject* shared_object_;
  mutable Mutex shared_object_mu_;

  DISALLOW_COPY_AND_ASSIGN(PeerObjectImpl);
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_PEER_OBJECT_IMPL_H_
