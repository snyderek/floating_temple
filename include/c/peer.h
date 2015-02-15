/*
 * Floating Temple
 * Copyright 2015 Derek S. Snyder
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INCLUDE_C_PEER_H_
#define INCLUDE_C_PEER_H_

#ifdef __cplusplus
extern "C" {
#endif

struct floatingtemple_DeserializationContext;
struct floatingtemple_Interpreter;
struct floatingtemple_LocalObject;
struct floatingtemple_Peer;
struct floatingtemple_PeerObject;
struct floatingtemple_SerializationContext;
struct floatingtemple_Thread;
struct floatingtemple_Value;

/* This function does not take ownership of *interpreter. The caller must take
 * ownership of the returned floatingtemple_Peer instance, and later free it by
 * calling floatingtemple_FreePeer. */
struct floatingtemple_Peer* floatingtemple_CreateNetworkPeer(
    const char* interpreter_type,
    int peer_port,
    int known_peer_id_count,
    const char** known_peer_ids,
    int send_receive_thread_count);

/* The caller must take ownership of the returned floatingtemple_Peer instance,
 * and later free it by calling floatingtemple_FreePeer. */
struct floatingtemple_Peer* floatingtemple_CreateStandalonePeer();

void floatingtemple_RunProgram(struct floatingtemple_Interpreter* interpreter,
                               struct floatingtemple_Peer* peer,
                               struct floatingtemple_LocalObject* local_object,
                               const char* method_name,
                               struct floatingtemple_Value* return_value);

/* Shuts down a floatingtemple_Peer instance previously returned by
 * floatingtemple_CreateNetworkPeer or floatingtemple_CreateStandalonePeer. */
void floatingtemple_StopPeer(struct floatingtemple_Peer* peer);

/* Frees the memory allocated for a floatingtemple_Peer instance. */
void floatingtemple_FreePeer(struct floatingtemple_Peer* peer);

/* Begins a transaction in the specified thread. Transactions may be nested by
 * calling floatingtemple_BeginTransaction more than once with the same thread
 * parameter, without an intervening call to floatingtemple_EndTransaction.
 * Method calls executed within a transaction will not be propagated to remote
 * peers until the outermost transaction is committed.
 *
 * Returns non-zero if the operation was successful. Returns zero if a conflict
 * occurred with another peer.
 *
 * IMPORTANT: If BeginTransaction returns zero, the caller must immediately
 * return from floatingtemple_Interpreter::invoke_method. */
int floatingtemple_BeginTransaction(struct floatingtemple_Thread* thread);

/* Ends the pending transaction that was begun most recently in the specified
 * thread. If that transaction is the outermost transaction, the transaction
 * will be committed and the method calls that were executed within the
 * transaction will be propagated to remote peers.
 *
 * Returns non-zero if the operation was successful. Returns zero if a conflict
 * occurred with another peer.
 *
 * IMPORTANT: If EndTransaction returns zero, the caller must immediately return
 * from floatingtemple_Interpreter::invoke_method. */
int floatingtemple_EndTransaction(struct floatingtemple_Thread* thread);

/* Returns a pointer to a newly created peer object that corresponds to an
 * existing local object. *initial_version is the initial version of the local
 * object; it may be cloned later via
 * floatingtemple_Interpreter::clone_local_object to create additional versions
 * of the object.
 *
 * The peer takes ownership of *initial_version. The caller must not take
 * ownership of the returned floatingtemple_PeerObject instance. */
struct floatingtemple_PeerObject* floatingtemple_CreatePeerObject(
    struct floatingtemple_Thread* thread,
    struct floatingtemple_LocalObject* initial_version);

/* Returns a pointer to the named object with the given name. If the named
 * object does not exist, it will be created using the initial_version
 * parameter, in a manner similar to floatingtemple_CreatePeerObject.
 *
 * The peer takes ownership of *initial_version. The caller must not take
 * ownership of the returned floatingtemple_PeerObject instance. */
struct floatingtemple_PeerObject* floatingtemple_GetOrCreateNamedObject(
    struct floatingtemple_Thread* thread, const char* name,
    struct floatingtemple_LocalObject* initial_version);

/* Calls the specified method on the specified object, and copies the return
 * value to *return_value. Depending on how the interpreted code is being
 * executed, floatingtemple_CallMethod may return a canned value instead of
 * actually calling the method. However, the local interpreter should not be
 * concerned about the details of this subterfuge.
 *
 * Returns non-zero if the method call was successful (possibly as a mock method
 * call). Returns zero if a conflict occurred with another peer.
 *
 * IMPORTANT: If floatingtemple_CallMethod returns zero, the caller must return
 * immediately from floatingtemple_Interpreter::invoke_method. */
int floatingtemple_CallMethod(struct floatingtemple_Interpreter* interpreter,
                              struct floatingtemple_Thread* thread,
                              struct floatingtemple_PeerObject* peer_object,
                              const char* method_name,
                              int parameter_count,
                              const struct floatingtemple_Value* parameters,
                              struct floatingtemple_Value* return_value);

/* Returns non-zero if the objects are equivalent. */
int floatingtemple_ObjectsAreEquivalent(
    const struct floatingtemple_Thread* thread,
    const struct floatingtemple_PeerObject* a,
    const struct floatingtemple_PeerObject* b);

int floatingtemple_GetSerializationIndexForPeerObject(
    struct floatingtemple_SerializationContext* context,
    struct floatingtemple_PeerObject* peer_object);

struct floatingtemple_PeerObject*
floatingtemple_GetPeerObjectBySerializationIndex(
    struct floatingtemple_DeserializationContext* context, int index);

int floatingtemple_PollForCallback(
    struct floatingtemple_Peer* peer,
    struct floatingtemple_Interpreter* interpreter);

void floatingtemple_TestFunction(int n, void (*callback)(int));

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* INCLUDE_C_PEER_H_ */
