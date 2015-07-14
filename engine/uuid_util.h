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

// The functions in this file are thread-safe.

#ifndef ENGINE_UUID_UTIL_H_
#define ENGINE_UUID_UTIL_H_

#include <string>

namespace floating_temple {
namespace peer {

class Uuid;

// Generates a 128-bit universally unique ID and stores it in the buffer pointed
// to by uuid, which must not be NULL.
void GenerateUuid(Uuid* uuid);

void GeneratePredictableUuid(const Uuid& ns_uuid, const std::string& name,
                             Uuid* uuid);

// Returns:
//   -1 if a < b
//    0 if a == b
//    1 if a > b
int CompareUuids(const Uuid& a, const Uuid& b);

// Returns a 32-character lower-case hexadecimal representation of uuid in big-
// endian order.
std::string UuidToString(const Uuid& uuid);

// Given a 32-character hexadecimal representation (upper or lower case) or a
// 128-bit unsigned integer in big-endian order, returns the UUID with that
// value.
Uuid StringToUuid(const std::string& s);

void UuidToHyphenatedString(const Uuid& uuid, std::string* s);

}  // namespace peer
}  // namespace floating_temple

namespace std {

// Define a less-than operator for Uuid so that it can be used as a key in
// std::map.
bool operator<(const floating_temple::peer::Uuid& a,
               const floating_temple::peer::Uuid& b);

}  // namespace std

#endif  // ENGINE_UUID_UTIL_H_
