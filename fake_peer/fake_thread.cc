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

#include "fake_peer/fake_thread.h"

#include <string>
#include <vector>

#include "base/linked_ptr.h"
#include "base/logging.h"
#include "base/string_printf.h"
#include "fake_peer/fake_peer_object.h"
#include "include/c++/local_object.h"

using std::string;
using std::vector;

namespace floating_temple {

FakeThread::FakeThread()
    : transaction_depth_(0) {
}

FakeThread::~FakeThread() {
  for (linked_ptr<PeerObject>& peer_object : peer_objects_) {
    VLOG(1) << "Deleting peer object " << StringPrintf("%p", peer_object.get());
    peer_object.reset(nullptr);
  }
}

bool FakeThread::BeginTransaction() {
  ++transaction_depth_;
  CHECK_GT(transaction_depth_, 0);
  return true;
}

bool FakeThread::EndTransaction() {
  CHECK_GT(transaction_depth_, 0);
  --transaction_depth_;
  return true;
}

PeerObject* FakeThread::CreatePeerObject(LocalObject* initial_version,
                                         const string& name) {
  // TODO(dss): If an object with the given name was already created, return a
  // pointer to the existing PeerObject instance.

  // TODO(dss): Implement garbage collection.

  PeerObject* const peer_object = new FakePeerObject(initial_version);
  VLOG(1) << "New peer object: " << StringPrintf("%p", peer_object);
  VLOG(1) << "peer_object: " << peer_object->Dump();
  peer_objects_.emplace_back(peer_object);
  return peer_object;
}

bool FakeThread::CallMethod(PeerObject* peer_object,
                            const string& method_name,
                            const vector<Value>& parameters,
                            Value* return_value) {
  CHECK(peer_object != nullptr);
  CHECK(!method_name.empty());
  CHECK(return_value != nullptr);

  VLOG(1) << "Calling method on peer object: "
          << StringPrintf("%p", peer_object);
  VLOG(1) << "peer_object: " << peer_object->Dump();

  FakePeerObject* const fake_peer_object = static_cast<FakePeerObject*>(
      peer_object);
  LocalObject* const local_object = fake_peer_object->local_object();

  VLOG(1) << "local_object: " << local_object->Dump();

  local_object->InvokeMethod(this, peer_object, method_name, parameters,
                             return_value);

  return true;
}

bool FakeThread::ObjectsAreEquivalent(const PeerObject* a,
                                      const PeerObject* b) const {
  CHECK(a != nullptr);
  CHECK(b != nullptr);

  return a == b;
}

}  // namespace floating_temple
