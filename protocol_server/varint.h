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

#ifndef PROTOCOL_SERVER_VARINT_H_
#define PROTOCOL_SERVER_VARINT_H_

#include "base/integral_types.h"

namespace floating_temple {

// The maximum possible length of a variable-length integer.
// ceil((64 bits) / (7 bits per byte))
const int kMaxVarintLength = 10;

// Parses a variable-length integer and stores it in *n. 'size' is the size of
// the buffer in bytes.
//
// If parsing was successful, this function returns the number of characters
// that were parsed. If the encoding of the variable-length integer is
// incomplete, this function returns -1 and leaves *n unchanged.
int ParseVarint(const char* buffer, int buffer_size, uint64* n);

// Encodes 'n' as a variable-length integer and stores it in the given buffer.
// 'size' is the size of the buffer in bytes. The buffer size must be at least
// GetVarintLength(n). (A buffer size of kMaxVarintLength will always be
// sufficient.)
//
// Returns the length of the encoding in bytes.
int FormatVarint(uint64 n, char* buffer, int buffer_size);

// Returns the number of bytes that are needed to encode the given integer
// value.
int GetVarintLength(uint64 n);

}  // namespace floating_temple

#endif  // PROTOCOL_SERVER_VARINT_H_
