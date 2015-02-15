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

#ifndef PEER_MOCK_LOCAL_OBJECT_H_
#define PEER_MOCK_LOCAL_OBJECT_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "include/c++/local_object.h"
#include "include/c++/value.h"
#include "third_party/gmock-1.7.0/include/gmock/gmock.h"

namespace floating_temple {

class PeerObject;
class SerializationContext;
class Thread;

namespace peer {

class MockLocalObjectCore {
 public:
  MockLocalObjectCore() {}

  MOCK_CONST_METHOD1(Serialize, std::string(SerializationContext* context));
  MOCK_CONST_METHOD5(InvokeMethod,
                     void(Thread* thread,
                          PeerObject* peer_object,
                          const std::string& method_name,
                          const std::vector<Value>& parameters,
                          Value* return_value));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockLocalObjectCore);
};

class MockLocalObject : public LocalObject {
 public:
  explicit MockLocalObject(const MockLocalObjectCore* core);

  virtual LocalObject* Clone() const;
  virtual std::size_t Serialize(void* buffer, std::size_t buffer_size,
                                SerializationContext* context) const;
  virtual void InvokeMethod(Thread* thread,
                            PeerObject* peer_object,
                            const std::string& method_name,
                            const std::vector<Value>& parameters,
                            Value* return_value);
  virtual std::string Dump() const;

 private:
  const MockLocalObjectCore* const core_;

  DISALLOW_COPY_AND_ASSIGN(MockLocalObject);
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_MOCK_LOCAL_OBJECT_H_
