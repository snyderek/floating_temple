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

#ifndef PEER_TRANSACTION_STORE_INTERNAL_INTERFACE_H_
#define PEER_TRANSACTION_STORE_INTERNAL_INTERFACE_H_

#include <string>
#include <tr1/unordered_map>
#include <vector>

#include "base/linked_ptr.h"
#include "peer/const_live_object_ptr.h"
#include "peer/live_object_ptr.h"

namespace floating_temple {
namespace peer {

class PeerObjectImpl;
class PendingEvent;
class SequencePoint;
class TransactionId;

class TransactionStoreInternalInterface {
 public:
  virtual ~TransactionStoreInternalInterface() {}

  // The caller must take ownership of the returned SequencePoint instance.
  virtual SequencePoint* GetCurrentSequencePoint() const = 0;

  virtual ConstLiveObjectPtr GetLiveObjectAtSequencePoint(
      PeerObjectImpl* peer_object, const SequencePoint* sequence_point,
      bool wait) = 0;

  virtual PeerObjectImpl* CreateUnboundPeerObject() = 0;
  virtual PeerObjectImpl* GetOrCreateNamedObject(const std::string& name) = 0;

  virtual void CreateTransaction(
      const std::vector<linked_ptr<PendingEvent> >& events,
      TransactionId* transaction_id,
      const std::tr1::unordered_map<PeerObjectImpl*, LiveObjectPtr>&
          modified_objects,
      const SequencePoint* prev_sequence_point) = 0;

  virtual bool ObjectsAreEquivalent(const PeerObjectImpl* a,
                                    const PeerObjectImpl* b) const = 0;
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_TRANSACTION_STORE_INTERNAL_INTERFACE_H_
