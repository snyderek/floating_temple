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

#ifndef INCLUDE_C_INTERPRETER_H_
#define INCLUDE_C_INTERPRETER_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct floatingtemple_DeserializationContext;
struct floatingtemple_PeerObject;
struct floatingtemple_SerializationContext;
struct floatingtemple_Thread;
struct floatingtemple_Value;

/* This struct represents one version of a particular object in the local
 * interpreter. It's simply a placeholder; the local interpreter should define
 * its own local object type and cast it to this type. */
struct floatingtemple_LocalObject {
};

/* The local interpreter should fill in the members of this struct and then pass
 * it to floatingtemple_PollForCallback (declared in "include/c/peer.h").
 *
 * The functions referenced in this struct must be thread-safe. */
struct floatingtemple_Interpreter {
  /* The local interpreter must return a pointer to a new
   * floatingtemple_LocalObject instance that is a clone of *local_object. The
   * peer will take ownership of the returned floatingtemple_LocalObject
   * instance. */
  struct floatingtemple_LocalObject* (*clone_local_object)(
      const struct floatingtemple_LocalObject* local_object);

  /* The peer calls this function to serialize a local interpreter object as a
   * string of bytes.
   *
   * buffer will point to a writable buffer, and buffer_size will be the maximum
   * number of bytes that can be written to the buffer.
   *
   * If the buffer is large enough, the local interpreter must serialize
   * *local_object to the buffer and return the number of bytes written.
   * Otherwise, it must leave the buffer untouched and return the minimum
   * required buffer size, in bytes.
   *
   * TODO(dss): Rename this member to serialize_object. */
  size_t (*serialize_local_object)(
      const struct floatingtemple_LocalObject* local_object,
      void* buffer,
      size_t buffer_size,
      struct floatingtemple_SerializationContext* context);

  /* The peer calls this function to deserialize an object and create it in the
   * local interpreter.
   *
   * buffer will point to a buffer that contains the serialized form of the
   * object. buffer_size will be the size of the buffer in bytes.
   *
   * The local interpreter must return a pointer to a newly created local
   * object. The peer will take ownership of this object. */
  struct floatingtemple_LocalObject* (*deserialize_object)(
      const void* buffer,
      size_t buffer_size,
      struct floatingtemple_DeserializationContext* context);

  /* The peer calls this function to free objects created by the local
   * interpreter. */
  void (*free_local_object)(struct floatingtemple_LocalObject* local_object);

  /* The peer calls this function to call a particular method on a particular
   * version of an object. method_name will be a null-terminated string; it will
   * not be the empty string.
   *
   * If the method executes successfully, the local interpreter must place the
   * method return value in *return_value. On the other hand, if a call to
   * floatingtemple_CallMethod returned zero during execution of the method
   * (because a conflict occurred), then *return_value will be ignored.
   *
   * The local interpreter must not store the floatingtemple_Thread pointer or
   * take ownership of the floatingtemple_Thread instance. */
  void (*invoke_method)(struct floatingtemple_LocalObject* local_object,
                        struct floatingtemple_Thread* thread,
                        struct floatingtemple_PeerObject* peer_object,
                        const char* method_name,
                        int parameter_count,
                        const struct floatingtemple_Value* parameters,
                        struct floatingtemple_Value* return_value);
};

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* INCLUDE_C_INTERPRETER_H_ */
