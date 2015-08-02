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

#ifndef UTIL_COMMA_SEPARATED_H_
#define UTIL_COMMA_SEPARATED_H_

#include <string>
#include <vector>

namespace floating_temple {

// Parses a comma-separated string into substrings and stores the substrings in
// the given vector. The comma characters themselves are not considered part of
// the substrings. White space characters are not trimmed. Zero-length
// substrings are ignored.
void ParseCommaSeparatedList(const std::string& in,
                             std::vector<std::string>* out);

}  // namespace floating_temple

#endif  // UTIL_COMMA_SEPARATED_H_
