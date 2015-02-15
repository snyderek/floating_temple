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

#include "base/escape.h"

#include <cctype>
#include <string>

#include "base/string_printf.h"

using std::isprint;
using std::string;

namespace floating_temple {
namespace {

string CEscapeChar(char c) {
  switch (c) {
    case '\0':
      return "\\0";
    case '\a':
      return "\\a";
    case '\b':
      return "\\b";
    case '\t':
      return "\\t";
    case '\n':
      return "\\n";
    case '\v':
      return "\\v";
    case '\f':
      return "\\f";
    case '\r':
      return "\\r";
    case '"':
      return "\\\"";
    case '\\':
      return "\\\\";

    default:
      if (isprint(c)) {
        return string(1, c);
      } else {
        return StringPrintf(
                   "\\%03o",
                   static_cast<unsigned>(static_cast<unsigned char>(c)));
      }
  }
}

}  // namespace

string CEscape(const string& s) {
  string escaped;

  for (string::const_iterator it = s.begin(); it != s.end(); ++it) {
    escaped += CEscapeChar(*it);
  }

  return escaped;
}

}  // namespace floating_temple
