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

#include "protocol_server/varint.h"

#include <cstddef>

#include "base/integral_types.h"
#include "base/logging.h"

namespace floating_temple {
namespace {

const char* FindVarintLastChar(const char* buffer, int buffer_size) {
  const char* const end_ptr = buffer + buffer_size;

  for (const char* p = buffer; p != end_ptr; ++p) {
    const unsigned char byte = static_cast<unsigned char>(*p);
    if ((byte & 0x80) == 0) {
      return p;
    }
  }

  return NULL;
}

}  // namespace

int ParseVarint(const char* buffer, int buffer_size, uint64* n) {
  CHECK(buffer != NULL);
  CHECK_GE(buffer_size, 0);
  CHECK(n != NULL);

  const char* const last_char_ptr = FindVarintLastChar(buffer, buffer_size);
  if (last_char_ptr == NULL) {
    return -1;
  }

  uint64 result = 0;

  for (const char* p = last_char_ptr; p >= buffer; --p) {
    const unsigned char byte = static_cast<unsigned char>(*p);

    // TODO(dss): Fail gracefully if the remote peer sends a variable-length
    // integer that is too large.
    CHECK_LT(result, (static_cast<uint64>(1) << 57))
        << "Arithmetic overflow while parsing variable-length integer "
        << "(buffer_size == " << buffer_size << ")";

    result <<= 7;
    result |= static_cast<uint64>(byte & 0x7f);
  }

  *n = result;
  return last_char_ptr - buffer + 1;
}

int FormatVarint(uint64 n, char* buffer, int buffer_size) {
  CHECK(buffer != NULL);
  CHECK_GT(buffer_size, 0);

  const int length = GetVarintLength(n);
  CHECK_LE(length, buffer_size);

  char* const last_char_ptr = buffer + (length - 1);

  for (char* p = buffer; p <= last_char_ptr; ++p) {
    unsigned char byte = 0u;

    if (p != last_char_ptr) {
      byte |= 0x80;
    }

    byte |= static_cast<unsigned char>(n & 0x7f);
    n >>= 7;

    *p = static_cast<char>(byte);
  }

  return length;
}

int GetVarintLength(uint64 n) {
  int length = 1;
  while ((n >>= 7) != 0) {
    ++length;
  }

  return length;
}

}  // namespace floating_temple
