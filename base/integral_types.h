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

#ifndef BASE_INTEGRAL_TYPES_H_
#define BASE_INTEGRAL_TYPES_H_

#include <tr1/cstdint>

namespace floating_temple {

// Fixed-width integral types

typedef std::tr1::int8_t int8;
typedef std::tr1::int16_t int16;
typedef std::tr1::int32_t int32;
typedef std::tr1::int64_t int64;

typedef std::tr1::uint8_t uint8;
typedef std::tr1::uint16_t uint16;
typedef std::tr1::uint32_t uint32;
typedef std::tr1::uint64_t uint64;

const int8 kInt8Min = INT8_MIN;
const int8 kInt8Max = INT8_MAX;
const int16 kInt16Min = INT16_MIN;
const int16 kInt16Max = INT16_MAX;
const int32 kInt32Min = INT32_MIN;
const int32 kInt32Max = INT32_MAX;
const int64 kInt64Min = INT64_MIN;
const int64 kInt64Max = INT64_MAX;

const uint8 kUint8Min = 0;
const uint8 kUint8Max = UINT8_MAX;
const uint16 kUint16Min = 0;
const uint16 kUint16Max = UINT16_MAX;
const uint32 kUint32Min = 0;
const uint32 kUint32Max = UINT32_MAX;
const uint64 kUint64Min = 0;
const uint64 kUint64Max = UINT64_MAX;

}  // namespace floating_temple

#endif  // BASE_INTEGRAL_TYPES_H_
