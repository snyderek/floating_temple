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

#ifndef FAKE_INTERPRETER_FAKE_LOCAL_OBJECT_H_
#define FAKE_INTERPRETER_FAKE_LOCAL_OBJECT_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "include/c++/local_object.h"

namespace floating_temple {

class FakeLocalObject : public LocalObject {
 public:
  static const int kVoidLocalType;
  static const int kStringLocalType;
  static const int kObjectLocalType;

  explicit FakeLocalObject(const std::string& s) : s_(s) {}

  const std::string& s() const { return s_; }

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
  std::string s_;

  DISALLOW_COPY_AND_ASSIGN(FakeLocalObject);
};

}  // namespace floating_temple

#endif  // FAKE_INTERPRETER_FAKE_LOCAL_OBJECT_H_
