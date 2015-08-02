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

#ifndef BASE_HEX_H_
#define BASE_HEX_H_

namespace floating_temple {

// Returns the numeric value of the given hexadecimal digit, or -1 if the 'hex'
// parameter is not a valid hexadecimal digit. Valid input characters are [0-9],
// [A-F], and [a-f].
int ParseHexDigit(char hex);

}  // namespace floating_temple

#endif  // BASE_HEX_H_
