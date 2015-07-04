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

#ifndef PEER_LIVE_OBJECT_H_
#define PEER_LIVE_OBJECT_H_

#include <memory>
#include <string>
#include <vector>

#include "include/c++/value.h"

namespace floating_temple {

class LocalObject;
class Thread;

namespace peer {

class ObjectReferenceImpl;

class LiveObject {
 public:
  virtual ~LiveObject() {}

  virtual const LocalObject* local_object() const = 0;

  virtual std::shared_ptr<LiveObject> Clone() const = 0;
  virtual void Serialize(
      std::string* data,
      std::vector<ObjectReferenceImpl*>* object_references) const = 0;
  virtual void InvokeMethod(Thread* thread,
                            ObjectReferenceImpl* object_reference,
                            const std::string& method_name,
                            const std::vector<Value>& parameters,
                            Value* return_value) = 0;

  virtual std::string Dump() const = 0;
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_LIVE_OBJECT_H_
