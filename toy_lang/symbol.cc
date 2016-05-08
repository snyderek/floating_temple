// Floating Temple
// Copyright 2016 Derek S. Snyder
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

#include "toy_lang/symbol.h"

#include <string>

#include "base/logging.h"

using std::string;

namespace floating_temple {
namespace toy_lang {

Symbol::Symbol(const string& symbol_name)
    : symbol_name_(symbol_name) {
  CHECK(!symbol_name.empty());
}

string Symbol::DebugString() const {
  return symbol_name_;
}

}  // namespace toy_lang
}  // namespace floating_temple
