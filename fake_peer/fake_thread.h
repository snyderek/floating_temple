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

#ifndef FAKE_PEER_FAKE_THREAD_H_
#define FAKE_PEER_FAKE_THREAD_H_

#include <string>
#include <vector>

#include "base/linked_ptr.h"
#include "base/macros.h"
#include "include/c++/thread.h"

namespace floating_temple {

class PeerObject;

class FakeThread : public Thread {
 public:
  FakeThread();
  virtual ~FakeThread();

  virtual bool BeginTransaction();
  virtual bool EndTransaction();
  virtual PeerObject* CreatePeerObject(LocalObject* initial_version);
  virtual PeerObject* GetOrCreateNamedObject(const std::string& name,
                                             LocalObject* initial_version);
  virtual bool CallMethod(PeerObject* peer_object,
                          const std::string& method_name,
                          const std::vector<Value>& parameters,
                          Value* return_value);
  virtual bool ObjectsAreEquivalent(const PeerObject* a,
                                    const PeerObject* b) const;

 private:
  PeerObject* PrivateCreatePeerObject(LocalObject* initial_version);

  std::vector<linked_ptr<PeerObject>> peer_objects_;
  int transaction_depth_;

  DISALLOW_COPY_AND_ASSIGN(FakeThread);
};

}  // namespace floating_temple

#endif  // FAKE_PEER_FAKE_THREAD_H_
