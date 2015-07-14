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

#include "include/c++/create_peer.h"

#include <string>
#include <vector>

#include "engine/peer_impl.h"

using std::string;
using std::vector;

namespace floating_temple {

Peer* CreateNetworkPeer(Interpreter* interpreter,
                        const string& interpreter_type,
                        const string& local_address,
                        int peer_port,
                        const vector<string>& known_peer_ids,
                        int send_receive_thread_count,
                        bool delay_object_binding) {
  peer::PeerImpl* const peer = new peer::PeerImpl();
  peer->Start(interpreter, interpreter_type, local_address, peer_port,
              known_peer_ids, send_receive_thread_count, delay_object_binding);

  return peer;
}

}  // namespace floating_temple
