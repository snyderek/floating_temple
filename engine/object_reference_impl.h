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

#ifndef ENGINE_OBJECT_REFERENCE_IMPL_H_
#define ENGINE_OBJECT_REFERENCE_IMPL_H_

#include "base/macros.h"
#include "base/mutex.h"
#include "include/c++/object_reference.h"

namespace floating_temple {
namespace engine {

class SharedObject;

class ObjectReferenceImpl : public ObjectReference {
 public:
  ObjectReferenceImpl();
  ~ObjectReferenceImpl() override;

  const SharedObject* shared_object() const { return PrivateGetSharedObject(); }
  SharedObject* shared_object() { return PrivateGetSharedObject(); }

  SharedObject* SetSharedObjectIfUnset(SharedObject* shared_object);

  void Dump(DumpContext* dc) const override;

 private:
  SharedObject* PrivateGetSharedObject() const;

  SharedObject* shared_object_;
  mutable Mutex shared_object_mu_;

  DISALLOW_COPY_AND_ASSIGN(ObjectReferenceImpl);
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_OBJECT_REFERENCE_IMPL_H_
