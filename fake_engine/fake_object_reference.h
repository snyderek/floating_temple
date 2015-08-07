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

#ifndef FAKE_ENGINE_FAKE_OBJECT_REFERENCE_H_
#define FAKE_ENGINE_FAKE_OBJECT_REFERENCE_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "include/c++/object_reference.h"

namespace floating_temple {

class LocalObject;

class FakeObjectReference : public ObjectReference {
 public:
  explicit FakeObjectReference(LocalObject* local_object);
  ~FakeObjectReference() override;

  LocalObject* local_object() { return local_object_.get(); }

  void Dump(DumpContext* dc) const override;

 private:
  const std::unique_ptr<LocalObject> local_object_;

  DISALLOW_COPY_AND_ASSIGN(FakeObjectReference);
};

}  // namespace floating_temple

#endif  // FAKE_ENGINE_FAKE_OBJECT_REFERENCE_H_
