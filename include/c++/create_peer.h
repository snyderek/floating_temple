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

// The caller must take ownership of the returned Peer instance.
//
// If delay_object_binding is true, an object reference created by calling
// Thread::CreateVersionedObject or Thread::CreateUnversionedObject will not be
// bound to a shared object until the object reference is used. This improves
// performance, but it also means that the local interpreter must call
// Thread::ObjectsAreIdentical to determine if two ObjectReference pointers
// refer to the same shared object. If delay_object_binding is false, the local
// interpreter can safely assume that if two ObjectReference pointers have
// unequal pointer values, then the object references refer to different shared
// objects.
Peer* CreateNetworkPeer(Interpreter* interpreter,
                        const std::string& interpreter_type,
                        const std::string& local_address,
                        int peer_port,
                        const std::vector<std::string>& known_peer_ids,
                        int send_receive_thread_count,
                        bool delay_object_binding);

// The caller must take ownership of the returned Peer instance.
Peer* CreateStandalonePeer();

}  // namespace floating_temple

#endif  // INCLUDE_CPP_CREATE_PEER_H_
