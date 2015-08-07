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
#include "engine/canonical_peer.h"
#include "engine/committed_event.h"
#include "util/dump_context.h"

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

void SharedObjectTransaction::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("events");
  dc->BeginList();
  for (const linked_ptr<CommittedEvent>& event : events_) {
    event->Dump(dc);
  }
  dc->End();

  dc->AddString("origin_peer");
  if (origin_peer_ == nullptr) {
    dc->AddNull();
  } else {
    dc->AddString(origin_peer_->peer_id());
  }

  dc->End();
}

}  // namespace engine
}  // namespace floating_temple
