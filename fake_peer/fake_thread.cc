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
#include "fake_peer/fake_peer_object.h"
#include "include/c++/local_object.h"

using std::string;
using std::vector;

namespace floating_temple {

FakeThread::FakeThread()
    : transaction_depth_(0) {
}

FakeThread::~FakeThread() {
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

PeerObject* FakeThread::CreatePeerObject(LocalObject* initial_version) {
  return PrivateCreatePeerObject(initial_version);
}

PeerObject* FakeThread::GetOrCreateNamedObject(const string& name,
                                               LocalObject* initial_version) {
  return PrivateCreatePeerObject(initial_version);
}

bool FakeThread::CallMethod(PeerObject* peer_object,
                            const string& method_name,
                            const vector<Value>& parameters,
                            Value* return_value) {
  CHECK(peer_object != nullptr);
  CHECK(!method_name.empty());
  CHECK(return_value != nullptr);

  FakePeerObject* const fake_peer_object = static_cast<FakePeerObject*>(
      peer_object);
  LocalObject* const local_object = fake_peer_object->local_object();

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

PeerObject* FakeThread::PrivateCreatePeerObject(LocalObject* initial_version) {
  // TODO(dss): Implement garbage collection.
  PeerObject* const peer_object = new FakePeerObject(initial_version);
  peer_objects_.push_back(make_linked_ptr(peer_object));
  return peer_object;
}

}  // namespace floating_temple
