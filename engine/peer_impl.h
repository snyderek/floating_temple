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

#ifndef ENGINE_PEER_IMPL_H_
#define ENGINE_PEER_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "engine/canonical_peer_map.h"
#include "engine/connection_manager.h"
#include "engine/transaction_store.h"
#include "include/c++/peer.h"
#include "util/state_variable.h"

namespace floating_temple {

class Interpreter;

namespace engine {

class PeerImpl : public Peer {
 public:
  PeerImpl();
  ~PeerImpl() override;

  void Start(Interpreter* interpreter,
             const std::string& interpreter_type,
             const std::string& local_address,
             int peer_port,
             const std::vector<std::string>& known_peer_ids,
             int send_receive_thread_count);

  void RunProgram(LocalObject* local_object,
                  const std::string& method_name,
                  Value* return_value,
                  bool linger) override;
  void Stop() override;

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
  std::unique_ptr<TransactionStore> transaction_store_;

  StateVariable state_;

  DISALLOW_COPY_AND_ASSIGN(PeerImpl);
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_PEER_IMPL_H_
