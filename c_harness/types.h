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

#ifndef C_HARNESS_TYPES_H_
#define C_HARNESS_TYPES_H_

#include <memory>

#include "c_harness/proxy_interpreter.h"
#include "include/c/peer.h"

namespace floating_temple {
class DeserializationContext;
class Peer;
class SerializationContext;
class Thread;
}

// NOTE: struct floatingtemple_PeerObject is not defined. The
// floatingtemple_PeerObject pointer is computed by casting the
// floating_temple::PeerObject pointer.
//
// TODO(dss): Eliminate this kludge and create a proper definition for struct
// floatingtemple_PeerObject.

struct floatingtemple_DeserializationContext {
  floating_temple::DeserializationContext* context;
};

struct floatingtemple_Peer {
  std::unique_ptr<floating_temple::Peer> peer;
  floating_temple::c_harness::ProxyInterpreter proxy_interpreter;
};

struct floatingtemple_SerializationContext {
  floating_temple::SerializationContext* context;
};

struct floatingtemple_Thread {
  floating_temple::c_harness::ProxyInterpreter* proxy_interpreter;
  floating_temple::Thread* thread;
};

#endif  // C_HARNESS_TYPES_H_
