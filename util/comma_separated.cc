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

#include "util/comma_separated.h"

#include <cstddef>
#include <string>
#include <vector>

#include "base/logging.h"

using std::string;
using std::vector;

namespace floating_temple {

void ParseCommaSeparatedList(const string& in, vector<string>* out) {
  CHECK(out != NULL);

  out->clear();
  string item;

  for (string::const_iterator it = in.begin(); it != in.end(); ++it) {
    const char c = *it;

    if (c == ',') {
      if (!item.empty()) {
        out->push_back(item);
        item.clear();
      }
    } else {
      item += c;
    }
  }

  if (!item.empty()) {
    out->push_back(item);
  }
}

}  // namespace floating_temple
