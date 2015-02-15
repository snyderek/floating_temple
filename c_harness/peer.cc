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

#include "include/c/peer.h"

#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "c_harness/proxy_interpreter.h"
#include "c_harness/types.h"
#include "include/c/value.h"
#include "include/c++/create_peer.h"
#include "include/c++/deserialization_context.h"
#include "include/c++/peer.h"
#include "include/c++/serialization_context.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "util/tcp.h"

namespace floating_temple {
class LocalObject;
class PeerObject;
}

using floating_temple::CreateNetworkPeer;
using floating_temple::CreateStandalonePeer;
using floating_temple::GetLocalAddress;
using floating_temple::LocalObject;
using floating_temple::PeerObject;
using floating_temple::Value;
using floating_temple::c_harness::ProxyInterpreter;
using std::printf;
using std::string;
using std::vector;

floatingtemple_Peer* floatingtemple_CreateNetworkPeer(
    const char* interpreter_type,
    int peer_port,
    int known_peer_id_count,
    const char** known_peer_ids,
    int send_receive_thread_count) {
  CHECK(interpreter_type != NULL);
  CHECK(known_peer_ids != NULL);

  vector<string> known_peer_id_vector(
      static_cast<vector<string>::size_type>(known_peer_id_count));
  for (int i = 0; i < known_peer_id_count; ++i) {
    known_peer_id_vector[i] = known_peer_ids[i];
  }

  floatingtemple_Peer* const peer = new floatingtemple_Peer();
  peer->peer.reset(CreateNetworkPeer(&peer->proxy_interpreter, interpreter_type,
                                     GetLocalAddress(), peer_port,
                                     known_peer_id_vector,
                                     send_receive_thread_count));

  return peer;
}

floatingtemple_Peer* floatingtemple_CreateStandalonePeer() {
  floatingtemple_Peer* const peer = new floatingtemple_Peer();
  peer->peer.reset(CreateStandalonePeer());

  return peer;
}

void floatingtemple_RunProgram(floatingtemple_Interpreter* interpreter,
                               floatingtemple_Peer* peer,
                               floatingtemple_LocalObject* local_object,
                               const char* method_name,
                               floatingtemple_Value* return_value) {
  CHECK(peer != NULL);
  CHECK(method_name != NULL);

  ProxyInterpreter* const proxy_interpreter = &peer->proxy_interpreter;

  LocalObject* const proxy_local_object =
      proxy_interpreter->CreateProxyLocalObject(local_object);

  floatingtemple_Interpreter* const old_interpreter =
      proxy_interpreter->SetInterpreterForCurrentThread(interpreter);

  peer->peer->RunProgram(proxy_local_object, method_name,
                         reinterpret_cast<Value*>(return_value));

  proxy_interpreter->SetInterpreterForCurrentThread(old_interpreter);
}

void floatingtemple_StopPeer(floatingtemple_Peer* peer) {
  CHECK(peer != NULL);
  peer->peer->Stop();
}

void floatingtemple_FreePeer(floatingtemple_Peer* peer) {
  delete peer;
}

int floatingtemple_BeginTransaction(floatingtemple_Thread* thread) {
  CHECK(thread != NULL);
  return thread->thread->BeginTransaction() ? 1 : 0;
}

int floatingtemple_EndTransaction(floatingtemple_Thread* thread) {
  CHECK(thread != NULL);
  return thread->thread->EndTransaction() ? 1 : 0;
}

floatingtemple_PeerObject* floatingtemple_CreatePeerObject(
    floatingtemple_Thread* thread,
    floatingtemple_LocalObject* initial_version) {
  CHECK(thread != NULL);

  return reinterpret_cast<floatingtemple_PeerObject*>(
      thread->thread->CreatePeerObject(
          thread->proxy_interpreter->CreateProxyLocalObject(initial_version)));
}

floatingtemple_PeerObject* floatingtemple_GetOrCreateNamedObject(
    floatingtemple_Thread* thread, const char* name,
    floatingtemple_LocalObject* initial_version) {
  CHECK(thread != NULL);
  CHECK(name != NULL);

  return reinterpret_cast<floatingtemple_PeerObject*>(
      thread->thread->GetOrCreateNamedObject(
          name,
          thread->proxy_interpreter->CreateProxyLocalObject(initial_version)));
}

int floatingtemple_CallMethod(floatingtemple_Interpreter* interpreter,
                              floatingtemple_Thread* thread,
                              floatingtemple_PeerObject* peer_object,
                              const char* method_name,
                              int parameter_count,
                              const floatingtemple_Value* parameters,
                              floatingtemple_Value* return_value) {
  CHECK(thread != NULL);
  CHECK(method_name != NULL);
  CHECK(parameters != NULL);

  vector<Value> parameter_vector(parameter_count);

  for (int i = 0; i < parameter_count; ++i) {
    parameter_vector[i] = *reinterpret_cast<const Value*>(&parameters[i]);
  }

  ProxyInterpreter* const proxy_interpreter = thread->proxy_interpreter;

  floatingtemple_Interpreter* const old_interpreter =
      proxy_interpreter->SetInterpreterForCurrentThread(interpreter);

  const bool success = thread->thread->CallMethod(
      reinterpret_cast<PeerObject*>(peer_object), method_name, parameter_vector,
      reinterpret_cast<Value*>(return_value));

  proxy_interpreter->SetInterpreterForCurrentThread(old_interpreter);

  return success ? 1 : 0;
}

int floatingtemple_ObjectsAreEquivalent(
    const floatingtemple_Thread* thread,
    const floatingtemple_PeerObject* a,
    const floatingtemple_PeerObject* b) {
  CHECK(thread != NULL);

  return thread->thread->ObjectsAreEquivalent(
      reinterpret_cast<const PeerObject*>(a),
      reinterpret_cast<const PeerObject*>(b)) ? 1 : 0;
}

int floatingtemple_GetSerializationIndexForPeerObject(
    floatingtemple_SerializationContext* context,
    floatingtemple_PeerObject* peer_object) {
  CHECK(context != NULL);

  return context->context->GetIndexForPeerObject(
      reinterpret_cast<PeerObject*>(peer_object));
}

floatingtemple_PeerObject* floatingtemple_GetPeerObjectBySerializationIndex(
    floatingtemple_DeserializationContext* context, int index) {
  CHECK(context != NULL);

  return reinterpret_cast<floatingtemple_PeerObject*>(
      context->context->GetPeerObjectByIndex(index));
}

int floatingtemple_PollForCallback(floatingtemple_Peer* peer,
                                   floatingtemple_Interpreter* interpreter) {
  CHECK(peer != NULL);
  return peer->proxy_interpreter.PollForCallback(interpreter) ? 1 : 0;
}

void floatingtemple_TestFunction(int n, void (*callback)(int)) {
  printf("%d\n", n);
  if (n > 0) {
    (*callback)(n - 1);
  }
}
