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

#ifndef INCLUDE_CPP_CREATE_PEER_H_
#define INCLUDE_CPP_CREATE_PEER_H_

#include <string>
#include <vector>

namespace floating_temple {

class Interpreter;
class Peer;

// Creates the Floating Temple engine and returns a pointer to an object that
// represents the peer. (peer = engine + local interpreter)
//
// The high-level structure of a Floating Temple interpreter should be as
// follows:
//
//   1) Create an Interpreter instance to represent the local interpreter.
//   2) Call CreateNetworkPeer to create the Floating Temple engine.
//   3) Create a LocalObject instance to represent the program to be executed.
//   4) Call Peer::RunProgram and pass it a pointer to the LocalObject instance
//      that represents the program to be executed.
//   5) Call Peer::Stop.
//
// 'interpreter_type' is an opaque string that identifies the language being
// interpreted. It must not be empty. The purpose of this string is to provide a
// sanity check that all peers in a cluster are executing the same interpreted
// language. [TODO(dss): Currently, there's no check that all peers in a cluster
// are executing the same _program_. This might be a good feature to add at some
// point.]
//
// 'local_address' is the IPv4 or IPv6 address of the local host. It must not be
// empty. It can be a named address or a numeric address. The address must be
// sufficient to allow a remote host to connect to this host. (In other words,
// it must not be "localhost".) You can get this string by calling the
// GetLocalAddress function, declared in "util/tcp.h". The local address string
// will be part of the peer ID for the local peer.
//
// 'peer_port' is the port that the local peer's server will listen on. It must
// be a valid TCP port number. The port number will be part of the peer ID for
// the local peer.
//
// 'known_peer_ids' is a list of peer IDs of remote peers that the local peer
// should connect to at start-up. Once the local peer is up and running, it may
// discover additional remote peers and connect to them automatically. The
// engine will crash if it can't connect to one of the peers in
// 'known_peer_ids', or if 'known_peer_ids' contains the peer ID for the local
// peer. [TODO(dss): These should not be fatal errors. Instead, the engine
// should log an error and continue without the bad peer ID.]
//
// The format for peer IDs is as follows:
//
//   ip/<address>/port
//
// For example:
//
//   ip/192.168.0.1/1025
//   ip/ottawa/1109
//
// 'send_receive_thread_count' is the number of threads that should be used to
// read and write data on socket connections to remote peers. It must be at
// least 1.
//
// This function does not take ownership of the Interpreter instance. The caller
// must take ownership of the returned Peer instance.
Peer* CreateNetworkPeer(Interpreter* interpreter,
                        const std::string& interpreter_type,
                        const std::string& local_address,
                        int peer_port,
                        const std::vector<std::string>& known_peer_ids,
                        int send_receive_thread_count);

// The function is similar to CreateNetworkPeer, except that the newly created
// peer has no ability to connect to other peers. This is useful for testing the
// integration between the local interpreter and the engine.
//
// The caller must take ownership of the returned Peer instance.
Peer* CreateStandalonePeer();

}  // namespace floating_temple

#endif  // INCLUDE_CPP_CREATE_PEER_H_
