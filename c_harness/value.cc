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

#include "include/c/value.h"

#include <stddef.h>
#include <stdint.h>

#include <string>

#include "base/logging.h"
#include "base/macros.h"
#include "include/c++/value.h"
#include "include/c/peer.h"

namespace floating_temple {
class PeerObject;
}

using floating_temple::PeerObject;
using floating_temple::Value;
using std::string;

namespace {

const Value* GetValue(const floatingtemple_Value* value) {
  CHECK(value != NULL);
  return reinterpret_cast<const Value*>(value);
}

Value* GetValue(floatingtemple_Value* value) {
  CHECK(value != NULL);
  return reinterpret_cast<Value*>(value);
}

}  // namespace

COMPILE_ASSERT(sizeof(floatingtemple_Value) >= sizeof(Value),
               floatingtemple_Value_struct_is_too_small);

void floatingtemple_InitValue(floatingtemple_Value* value) {
  new(GetValue(value)) Value();
}

void floatingtemple_DestroyValue(floatingtemple_Value* value) {
  GetValue(value)->~Value();
}

void floatingtemple_InitValueArray(floatingtemple_Value* value_array,
                                   int count) {
  CHECK(value_array != NULL);
  CHECK_GE(count, 0);

  for (int i = 0; i < count; ++i) {
    floatingtemple_InitValue(&value_array[i]);
  }
}

int floatingtemple_GetValueLocalType(const floatingtemple_Value* value) {
  return GetValue(value)->local_type();
}

int floatingtemple_GetValueType(const floatingtemple_Value* value) {
  const Value::Type type = GetValue(value)->type();

  switch (type) {
    case Value::UNINITIALIZED: return VALUE_TYPE_UNINITIALIZED;
    case Value::EMPTY:         return VALUE_TYPE_EMPTY;
    case Value::DOUBLE:        return VALUE_TYPE_DOUBLE;
    case Value::FLOAT:         return VALUE_TYPE_FLOAT;
    case Value::INT64:         return VALUE_TYPE_INT64;
    case Value::UINT64:        return VALUE_TYPE_UINT64;
    case Value::BOOL:          return VALUE_TYPE_BOOL;
    case Value::STRING:        return VALUE_TYPE_STRING;
    case Value::BYTES:         return VALUE_TYPE_BYTES;
    case Value::PEER_OBJECT:   return VALUE_TYPE_PEER_OBJECT;

    default:
      LOG(FATAL) << "Invalid value type: " << static_cast<int>(type);
  }

  return 0;
}

double floatingtemple_GetValueDouble(const floatingtemple_Value* value) {
  return GetValue(value)->double_value();
}

float floatingtemple_GetValueFloat(const floatingtemple_Value* value) {
  return GetValue(value)->float_value();
}

int64_t floatingtemple_GetValueInt64(const floatingtemple_Value* value) {
  return GetValue(value)->int64_value();
}

uint64_t floatingtemple_GetValueUint64(const floatingtemple_Value* value) {
  return GetValue(value)->uint64_value();
}

int floatingtemple_GetValueBool(const floatingtemple_Value* value) {
  return GetValue(value)->bool_value();
}

const char* floatingtemple_GetValueStringData(
    const floatingtemple_Value* value) {
  return GetValue(value)->string_value().data();
}

size_t floatingtemple_GetValueStringLength(const floatingtemple_Value* value) {
  return GetValue(value)->string_value().length();
}

const char* floatingtemple_GetValueBytesData(
    const floatingtemple_Value* value) {
  return GetValue(value)->bytes_value().data();
}

size_t floatingtemple_GetValueBytesLength(const floatingtemple_Value* value) {
  return GetValue(value)->bytes_value().length();
}

floatingtemple_PeerObject* floatingtemple_GetValuePeerObject(
    const floatingtemple_Value* value) {
  return reinterpret_cast<floatingtemple_PeerObject*>(
      GetValue(value)->peer_object());
}

void floatingtemple_SetValueEmpty(floatingtemple_Value* value, int local_type) {
  GetValue(value)->set_empty(local_type);
}

void floatingtemple_SetValueDouble(floatingtemple_Value* value, int local_type,
                                   double d) {
  GetValue(value)->set_double_value(local_type, d);
}

void floatingtemple_SetValueFloat(floatingtemple_Value* value, int local_type,
                                  float f) {
  GetValue(value)->set_float_value(local_type, f);
}

void floatingtemple_SetValueInt64(floatingtemple_Value* value, int local_type,
                                  int64_t n) {
  GetValue(value)->set_int64_value(local_type, n);
}

void floatingtemple_SetValueUint64(floatingtemple_Value* value, int local_type,
                                   uint64_t n) {
  GetValue(value)->set_uint64_value(local_type, n);
}

void floatingtemple_SetValueBool(floatingtemple_Value* value, int local_type,
                                 int b) {
  GetValue(value)->set_bool_value(local_type, b);
}

void floatingtemple_SetValueString(floatingtemple_Value* value, int local_type,
                                   const char* data, size_t length) {
  GetValue(value)->set_string_value(
      local_type, string(data, static_cast<string::size_type>(length)));
}

void floatingtemple_SetValueBytes(floatingtemple_Value* value, int local_type,
                                  const char* data, size_t length) {
  GetValue(value)->set_bytes_value(
      local_type, string(data, static_cast<string::size_type>(length)));
}

void floatingtemple_SetValuePeerObject(floatingtemple_Value* value,
                                       int local_type,
                                       floatingtemple_PeerObject* peer_object) {
  GetValue(value)->set_peer_object(local_type,
                                   reinterpret_cast<PeerObject*>(peer_object));
}

void floatingtemple_AssignValue(floatingtemple_Value* dest,
                                const floatingtemple_Value* src) {
  *GetValue(dest) = *GetValue(src);
}
