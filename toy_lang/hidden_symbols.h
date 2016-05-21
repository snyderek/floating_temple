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

#ifndef TOY_LANG_HIDDEN_SYMBOLS_H_
#define TOY_LANG_HIDDEN_SYMBOLS_H_

namespace floating_temple {
namespace toy_lang {

struct HiddenSymbols {
  int get_variable_symbol_id;
  int set_variable_symbol_id;
};

}  // namespace toy_lang
}  // namespace floating_temple

#endif  // TOY_LANG_HIDDEN_SYMBOLS_H_
