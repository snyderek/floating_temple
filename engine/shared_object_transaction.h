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

#ifndef ENGINE_SHARED_OBJECT_TRANSACTION_H_
#define ENGINE_SHARED_OBJECT_TRANSACTION_H_

#include <memory>
#include <vector>

#include "base/macros.h"

namespace floating_temple {

class DumpContext;

namespace engine {

class CanonicalPeer;
class CommittedEvent;

// TODO(dss): Consider renaming this class. It no longer applies just to
// SharedObject instances.
class SharedObjectTransaction {
 public:
  SharedObjectTransaction(
      const std::vector<std::unique_ptr<CommittedEvent>>& events,
      const CanonicalPeer* origin_peer);
  explicit SharedObjectTransaction(const CanonicalPeer* origin_peer);
  ~SharedObjectTransaction();

  const std::vector<std::unique_ptr<CommittedEvent>>& events() const
      { return events_; }
  const CanonicalPeer* origin_peer() const { return origin_peer_; }

  void AddEvent(CommittedEvent* event);

  SharedObjectTransaction* Clone() const;

  void Dump(DumpContext* dc) const;

  // These members are defined to allow tests to iterate over the events in the
  // transaction.
  typedef std::unique_ptr<CommittedEvent> value_type;
  typedef std::vector<std::unique_ptr<CommittedEvent>>::const_iterator
      const_iterator;
  const_iterator begin() const { return events_.begin(); }
  const_iterator end() const { return events_.end(); }

 private:
  std::vector<std::unique_ptr<CommittedEvent>> events_;
  const CanonicalPeer* const origin_peer_;

  DISALLOW_COPY_AND_ASSIGN(SharedObjectTransaction);
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_SHARED_OBJECT_TRANSACTION_H_
