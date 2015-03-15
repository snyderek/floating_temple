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

#ifndef PEER_PEER_IMPL_H_
#define PEER_PEER_IMPL_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/scoped_ptr.h"
#include "include/c++/peer.h"
#include "peer/canonical_peer_map.h"
#include "peer/connection_manager.h"
#include "peer/transaction_store.h"
#include "util/state_variable.h"

namespace floating_temple {

class Interpreter;

namespace peer {

class PeerImpl : public Peer {
 public:
  PeerImpl();
  virtual ~PeerImpl();

  void Start(Interpreter* interpreter,
             const std::string& interpreter_type,
             const std::string& local_address,
             int peer_port,
             const std::vector<std::string>& known_peer_ids,
             int send_receive_thread_count,
             bool delay_object_binding);

  virtual void RunProgram(LocalObject* local_object,
                          const std::string& method_name, Value* return_value);
  virtual void Stop();

 private:
  enum {
    NOT_STARTED = 0x1,
    STARTING = 0x2,
    RUNNING = 0x4,
    STOPPING = 0x8,
    STOPPED = 0x10
  };

  CanonicalPeerMap canonical_peer_map_;
  ConnectionManager connection_manager_;
  scoped_ptr<TransactionStore> transaction_store_;

  StateVariable state_;

  DISALLOW_COPY_AND_ASSIGN(PeerImpl);
};

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_PEER_IMPL_H_
