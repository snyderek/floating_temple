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

#ifndef FAKE_ENGINE_FAKE_METHOD_CONTEXT_H_
#define FAKE_ENGINE_FAKE_METHOD_CONTEXT_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "include/c++/method_context.h"

namespace floating_temple {

class ObjectReference;

class FakeMethodContext : public MethodContext {
 public:
  FakeMethodContext();
  ~FakeMethodContext() override;

  bool BeginTransaction() override;
  bool EndTransaction() override;
  ObjectReference* CreateObject(LocalObject* initial_version,
                                const std::string& name) override;
  bool CallMethod(ObjectReference* object_reference,
                  const std::string& method_name,
                  const std::vector<Value>& parameters,
                  Value* return_value) override;
  bool ObjectsAreIdentical(const ObjectReference* a,
                           const ObjectReference* b) const override;

 private:
  std::vector<std::unique_ptr<ObjectReference>> object_references_;
  int transaction_depth_;

  DISALLOW_COPY_AND_ASSIGN(FakeMethodContext);
};

}  // namespace floating_temple

#endif  // FAKE_ENGINE_FAKE_METHOD_CONTEXT_H_
