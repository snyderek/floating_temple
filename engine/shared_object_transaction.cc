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

#include "engine/shared_object_transaction.h"

#include <string>
#include <vector>

#include "base/escape.h"
#include "base/linked_ptr.h"
#include "base/logging.h"
#include "base/string_printf.h"
#include "engine/canonical_peer.h"
#include "engine/committed_event.h"

using std::string;
using std::vector;

namespace floating_temple {
namespace engine {

SharedObjectTransaction::SharedObjectTransaction(
    const vector<linked_ptr<CommittedEvent>>& events,
    const CanonicalPeer* origin_peer)
    : origin_peer_(CHECK_NOTNULL(origin_peer)) {
  const vector<linked_ptr<CommittedEvent>>::size_type event_count =
      events.size();
  events_.resize(event_count);

  for (vector<linked_ptr<CommittedEvent>>::size_type i = 0; i < event_count;
       ++i) {
    events_[i].reset(events[i]->Clone());
  }
}

SharedObjectTransaction::SharedObjectTransaction(
    const CanonicalPeer* origin_peer)
    : origin_peer_(CHECK_NOTNULL(origin_peer)) {
}

SharedObjectTransaction::~SharedObjectTransaction() {
}

void SharedObjectTransaction::AddEvent(CommittedEvent* event) {
  CHECK(event != nullptr);
  events_.emplace_back(event);
}

SharedObjectTransaction* SharedObjectTransaction::Clone() const {
  return new SharedObjectTransaction(events_, origin_peer_);
}

string SharedObjectTransaction::Dump() const {
  string events_string;

  if (events_.empty()) {
    events_string = "[]";
  } else {
    events_string = "[";

    for (vector<linked_ptr<CommittedEvent>>::const_iterator it =
             events_.begin();
         it != events_.end(); ++it) {
      if (it != events_.begin()) {
        events_string += ",";
      }

      StringAppendF(&events_string, " %s", (*it)->Dump().c_str());
    }

    events_string += " ]";
  }

  string origin_peer_id_string;
  if (origin_peer_ == nullptr) {
    origin_peer_id_string = "null";
  } else {
    SStringPrintf(&origin_peer_id_string, "\"%s\"",
                  CEscape(origin_peer_->peer_id()).c_str());
  }

  return StringPrintf("{ \"events\": %s, \"origin_peer\": %s }",
                      events_string.c_str(), origin_peer_id_string.c_str());
}

}  // namespace engine
}  // namespace floating_temple
