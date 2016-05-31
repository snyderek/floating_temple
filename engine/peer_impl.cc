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

#include "engine/peer_impl.h"

#include <memory>
#include <string>
#include <vector>

#include "base/logging.h"
#include "engine/canonical_peer_map.h"
#include "engine/connection_manager.h"
#include "engine/peer_id.h"
#include "engine/transaction_store.h"
#include "util/state_variable.h"

using std::string;
using std::vector;

namespace floating_temple {

class ObjectReference;

namespace engine {

class CanonicalPeer;
class SharedObject;

PeerImpl::PeerImpl()
    : state_(NOT_STARTED)  {
  state_.AddStateTransition(NOT_STARTED, STARTING);
  state_.AddStateTransition(STARTING, RUNNING);
  state_.AddStateTransition(RUNNING, STOPPING);
  state_.AddStateTransition(STOPPING, STOPPED);
}

PeerImpl::~PeerImpl() {
  state_.CheckState(NOT_STARTED | STOPPED);
}

void PeerImpl::Start(Interpreter* interpreter,
                     const string& interpreter_type,
                     const string& local_address,
                     int peer_port,
                     const vector<string>& known_peer_ids,
                     int send_receive_thread_count) {
  CHECK(interpreter != nullptr);

  state_.ChangeState(STARTING);

  const string local_peer_id = MakePeerId(local_address, peer_port);
  LOG(INFO) << "The local peer id is " << local_peer_id;

  const CanonicalPeer* const local_peer = canonical_peer_map_.GetCanonicalPeer(
      local_peer_id);

  transaction_store_.reset(new TransactionStore(&canonical_peer_map_,
                                                &connection_manager_,
                                                interpreter, local_peer));
  connection_manager_.Start(&canonical_peer_map_, interpreter_type, local_peer,
                            transaction_store_.get(),
                            send_receive_thread_count);

  // Connect to remote peers.
  for (const string& peer_id : known_peer_ids) {
    const CanonicalPeer* const known_peer =
        canonical_peer_map_.GetCanonicalPeer(peer_id);
    connection_manager_.ConnectToRemotePeer(known_peer);
  }

  state_.ChangeState(RUNNING);
}

void PeerImpl::RunProgram(LocalObject* local_object,
                          const string& method_name,
                          Value* return_value,
                          bool linger) {
  if (state_.WaitForNotState(NOT_STARTED | STARTING) != RUNNING) {
    return;
  }

  transaction_store_->RunProgram(local_object, method_name, return_value,
                                 linger);
}

void PeerImpl::Stop() {
  state_.ChangeState(STOPPING);

  connection_manager_.Stop();
  transaction_store_.reset(nullptr);

  state_.ChangeState(STOPPED);
}

}  // namespace engine
}  // namespace floating_temple
