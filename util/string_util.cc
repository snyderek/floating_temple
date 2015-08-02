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

#include "util/string_util.h"

#include <cstring>
#include <string>

using std::memcpy;
using std::string;

namespace floating_temple {

char* CreateCharBuffer(const string& s) {
  const string::size_type size = s.length() + 1;
  char* const buffer = new char[size];
  memcpy(buffer, s.c_str(), size);

  return buffer;
}

}  // namespace floating_temple
