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

#ifndef ENGINE_LIVE_OBJECT_H_
#define ENGINE_LIVE_OBJECT_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/mutex.h"
#include "include/c++/value.h"

namespace floating_temple {

class DumpContext;
class LocalObject;
class MethodContext;

namespace engine {

class LiveObjectNode;
class ObjectReferenceImpl;

class LiveObject {
 public:
  explicit LiveObject(LocalObject* local_object);
  ~LiveObject();

  const LocalObject* local_object() const;

  std::shared_ptr<LiveObject> Clone() const;
  void Serialize(std::string* data,
                 std::vector<ObjectReferenceImpl*>* object_references) const;
  void InvokeMethod(MethodContext* method_context,
                    ObjectReferenceImpl* self_object_reference,
                    const std::string& method_name,
                    const std::vector<Value>& parameters,
                    Value* return_value);

  void Dump(DumpContext* dc) const;

 private:
  explicit LiveObject(LiveObjectNode* node);

  LiveObjectNode* GetNode() const;

  LiveObjectNode* node_;  // Not NULL
  mutable Mutex node_mu_;

  DISALLOW_COPY_AND_ASSIGN(LiveObject);
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_LIVE_OBJECT_H_
