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

#ifndef BASE_ESCAPE_H_
#define BASE_ESCAPE_H_

#include <string>

namespace floating_temple {

// Returns a representation of the parameter as a C-style string (without the
// enclosing double quotes). Double quotes, backslashes, and non-printable
// characters are escaped.
std::string CEscape(const std::string& s);

}  // namespace floating_temple

#endif  // BASE_ESCAPE_H_
