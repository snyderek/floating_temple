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

#ifndef PEER_OBJECT_REFERENCE_IMPL_H_
#define PEER_OBJECT_REFERENCE_IMPL_H_

#include <string>

#include "base/macros.h"
#include "base/mutex.h"
#include "include/c++/object_reference.h"

namespace floating_temple {
namespace peer {

class SharedObject;

class ObjectReferenceImpl : public ObjectReference {
 public:
  explicit ObjectReferenceImpl(bool versioned);
  ~ObjectReferenceImpl() override;

  bool versioned() const { return versioned_; }

  const SharedObject* shared_object() const { return PrivateGetSharedObject(); }
  SharedObject* shared_object() { return PrivateGetSharedObject(); }

  SharedObject* SetSharedObjectIfUnset(SharedObject* shared_object);

  std::string Dump() const override;

 private:
  SharedObject* PrivateGetSharedObject() const;

  const bool versioned_;

  SharedObject* shared_object_;
  mutable Mutex shared_object_mu_;

  DISALLOW_COPY_AND_ASSIGN(ObjectReferenceImpl);
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_OBJECT_REFERENCE_IMPL_H_
