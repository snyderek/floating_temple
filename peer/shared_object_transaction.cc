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

#include "peer/shared_object_transaction.h"

#include <cstddef>
#include <string>
#include <vector>

#include "base/escape.h"
#include "base/linked_ptr.h"
#include "base/logging.h"
#include "base/string_printf.h"
#include "peer/canonical_peer.h"
#include "peer/committed_event.h"

using std::string;
using std::vector;

namespace floating_temple {
namespace peer {

SharedObjectTransaction::SharedObjectTransaction(
    vector<linked_ptr<CommittedEvent> >* events,
    const CanonicalPeer* origin_peer)
    : origin_peer_(CHECK_NOTNULL(origin_peer)) {
  CHECK(events != NULL);
  events_.swap(*events);
}

SharedObjectTransaction::~SharedObjectTransaction() {
}

string SharedObjectTransaction::Dump() const {
  string events_string;

  if (events_.empty()) {
    events_string = "[]";
  } else {
    events_string = "[";

    for (vector<linked_ptr<CommittedEvent> >::const_iterator it =
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
  if (origin_peer_ == NULL) {
    origin_peer_id_string = "null";
  } else {
    SStringPrintf(&origin_peer_id_string, "\"%s\"",
                  CEscape(origin_peer_->peer_id()).c_str());
  }

  return StringPrintf("{ \"events\": %s, \"origin_peer\": %s }",
                      events_string.c_str(), origin_peer_id_string.c_str());
}

}  // namespace peer
}  // namespace floating_temple
