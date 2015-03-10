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

#ifndef INCLUDE_C_VALUE_H_
#define INCLUDE_C_VALUE_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct floatingtemple_PeerObject;

/* NOTE: If you change the definition of this struct, remember to change the
 * definition of the corresponding VALUE PyPy object (in
 * third_party/pypy-2.4.0-src/pypy/objspace/floating_temple/peer_ffi.py). */
struct floatingtemple_Value {
  union {
    // Ensure that the struct is large enough.
    char padding[24];

    // Ensure that the struct is properly aligned.
    int64_t alignment1;
    double alignment2;
    float alignment3;
    void* alignment4;
  };
};

enum {
  VALUE_TYPE_UNINITIALIZED,
  VALUE_TYPE_EMPTY,
  VALUE_TYPE_DOUBLE,
  VALUE_TYPE_FLOAT,
  VALUE_TYPE_INT64,
  VALUE_TYPE_UINT64,
  VALUE_TYPE_BOOL,
  VALUE_TYPE_STRING,
  VALUE_TYPE_BYTES,
  VALUE_TYPE_PEER_OBJECT
};

void floatingtemple_InitValue(struct floatingtemple_Value* value);
void floatingtemple_DestroyValue(struct floatingtemple_Value* value);

void floatingtemple_InitValueArray(struct floatingtemple_Value* value_array,
                                   int count);

/* Returns the integer that was passed in the local_type parameter when the
 * value was set. */
int floatingtemple_GetValueLocalType(const struct floatingtemple_Value* value);

/* Returns the type of value stored in *value. (The return value will be one of
 * the enum constants defined above.) */
int floatingtemple_GetValueType(const struct floatingtemple_Value* value);

/* These accessor functions return the value stored in *value, depending on the
 * type of value. You must call floatingtemple_GetValueType(value) first to
 * determine which accessor function to call. Calling the wrong function will
 * cause a crash. Note that the VALUE_TYPE_UNINITIALIZED and VALUE_TYPE_EMPTY
 * types do not have values. */

/* VALUE_TYPE_DOUBLE */
double floatingtemple_GetValueDouble(const struct floatingtemple_Value* value);

/* VALUE_TYPE_FLOAT */
float floatingtemple_GetValueFloat(const struct floatingtemple_Value* value);

/* VALUE_TYPE_INT64 */
int64_t floatingtemple_GetValueInt64(const struct floatingtemple_Value* value);

/* VALUE_TYPE_UINT64 */
uint64_t floatingtemple_GetValueUint64(
    const struct floatingtemple_Value* value);

/* VALUE_TYPE_BOOL */
int floatingtemple_GetValueBool(const struct floatingtemple_Value* value);

/* VALUE_TYPE_STRING */
const char* floatingtemple_GetValueStringData(
    const struct floatingtemple_Value* value);
size_t floatingtemple_GetValueStringLength(
    const struct floatingtemple_Value* value);

/* VALUE_TYPE_BYTES */
const char* floatingtemple_GetValueBytesData(
    const struct floatingtemple_Value* value);
size_t floatingtemple_GetValueBytesLength(
    const struct floatingtemple_Value* value);

/* VALUE_TYPE_PEER_OBJECT */
struct floatingtemple_PeerObject* floatingtemple_GetValuePeerObject(
    const struct floatingtemple_Value* value);

void floatingtemple_SetValueEmpty(struct floatingtemple_Value* value,
                                  int local_type);
void floatingtemple_SetValueDouble(struct floatingtemple_Value* value,
                                   int local_type, double d);
void floatingtemple_SetValueFloat(struct floatingtemple_Value* value,
                                  int local_type, float f);
void floatingtemple_SetValueInt64(struct floatingtemple_Value* value,
                                  int local_type, int64_t n);
void floatingtemple_SetValueUint64(struct floatingtemple_Value* value,
                                   int local_type, uint64_t n);
void floatingtemple_SetValueBool(struct floatingtemple_Value* value,
                                 int local_type, int b);
void floatingtemple_SetValueString(struct floatingtemple_Value* value,
                                   int local_type, const char* data,
                                   size_t length);
void floatingtemple_SetValueBytes(struct floatingtemple_Value* value,
                                  int local_type, const char* data,
                                  size_t length);
/* peer_object must not be NULL. */
void floatingtemple_SetValuePeerObject(
    struct floatingtemple_Value* value, int local_type,
    struct floatingtemple_PeerObject* peer_object);

void floatingtemple_AssignValue(struct floatingtemple_Value* dest,
                                const struct floatingtemple_Value* src);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* INCLUDE_C_VALUE_H_ */
