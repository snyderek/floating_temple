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

#include "peer/pending_event.h"

#include <cstddef>
#include <string>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "include/c++/value.h"
#include "peer/const_live_object_ptr.h"
#include "util/stl_util.h"

using std::make_pair;
using std::string;
using std::tr1::unordered_map;
using std::tr1::unordered_set;
using std::vector;

namespace floating_temple {
namespace peer {

PendingEvent::PendingEvent(
    const unordered_map<PeerObjectImpl*, ConstLiveObjectPtr>& live_objects,
    const unordered_set<PeerObjectImpl*>& new_peer_objects,
    PeerObjectImpl* prev_peer_object)
    : live_objects_(live_objects),
      new_peer_objects_(new_peer_objects),
      prev_peer_object_(prev_peer_object) {
  // Check that new_peer_objects is a subset of keys(live_objects).
  for (PeerObjectImpl* const peer_object : new_peer_objects) {
    CHECK(live_objects.find(peer_object) != live_objects.end());
  }
}

void PendingEvent::GetMethodCall(PeerObjectImpl** next_peer_object,
                                 const string** method_name,
                                 const vector<Value>** parameters) const {
  LOG(FATAL) << "Invalid call to GetMethodCall (type == "
             << static_cast<int>(this->type()) << ")";
}

void PendingEvent::GetMethodReturn(PeerObjectImpl** next_peer_object,
                                   const Value** return_value) const {
  LOG(FATAL) << "Invalid call to GetMethodReturn (type == "
             << static_cast<int>(this->type()) << ")";
}

ObjectCreationPendingEvent::ObjectCreationPendingEvent(
    PeerObjectImpl* prev_peer_object, PeerObjectImpl* new_peer_object,
    const ConstLiveObjectPtr& new_live_object)
    : PendingEvent(
          MakeSingletonSet<unordered_map<PeerObjectImpl*, ConstLiveObjectPtr> >(
              make_pair(CHECK_NOTNULL(new_peer_object), new_live_object)),
          MakeSingletonSet<unordered_set<PeerObjectImpl*> >(new_peer_object),
          prev_peer_object) {
}

BeginTransactionPendingEvent::BeginTransactionPendingEvent(
    PeerObjectImpl* prev_peer_object)
    : PendingEvent(unordered_map<PeerObjectImpl*, ConstLiveObjectPtr>(),
                   unordered_set<PeerObjectImpl*>(),
                   CHECK_NOTNULL(prev_peer_object)) {
}

EndTransactionPendingEvent::EndTransactionPendingEvent(
    PeerObjectImpl* prev_peer_object)
    : PendingEvent(unordered_map<PeerObjectImpl*, ConstLiveObjectPtr>(),
                   unordered_set<PeerObjectImpl*>(),
                   CHECK_NOTNULL(prev_peer_object)) {
}

MethodCallPendingEvent::MethodCallPendingEvent(
    const unordered_map<PeerObjectImpl*, ConstLiveObjectPtr>& live_objects,
    const unordered_set<PeerObjectImpl*>& new_peer_objects,
    PeerObjectImpl* prev_peer_object,
    PeerObjectImpl* next_peer_object,
    const string& method_name,
    const vector<Value>& parameters)
    : PendingEvent(live_objects, new_peer_objects, prev_peer_object),
      next_peer_object_(CHECK_NOTNULL(next_peer_object)),
      method_name_(method_name),
      parameters_(parameters) {
  CHECK(!method_name.empty());
}

void MethodCallPendingEvent::GetMethodCall(
    PeerObjectImpl** next_peer_object, const string** method_name,
    const vector<Value>** parameters) const {
  CHECK(next_peer_object != NULL);
  CHECK(method_name != NULL);
  CHECK(parameters != NULL);

  *next_peer_object = next_peer_object_;
  *method_name = &method_name_;
  *parameters = &parameters_;
}

MethodReturnPendingEvent::MethodReturnPendingEvent(
    const unordered_map<PeerObjectImpl*, ConstLiveObjectPtr>& live_objects,
    const unordered_set<PeerObjectImpl*>& new_peer_objects,
    PeerObjectImpl* prev_peer_object,
    PeerObjectImpl* next_peer_object,
    const Value& return_value)
    : PendingEvent(live_objects, new_peer_objects,
                   CHECK_NOTNULL(prev_peer_object)),
      next_peer_object_(next_peer_object),
      return_value_(return_value) {
}

void MethodReturnPendingEvent::GetMethodReturn(
    PeerObjectImpl** next_peer_object, const Value** return_value) const {
  CHECK(next_peer_object != NULL);
  CHECK(return_value != NULL);

  *next_peer_object = next_peer_object_;
  *return_value = &return_value_;
}

}  // namespace peer
}  // namespace floating_temple
