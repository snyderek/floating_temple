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

#ifndef TOY_LANG_RUN_TOY_LANG_PROGRAM_H_
#define TOY_LANG_RUN_TOY_LANG_PROGRAM_H_

#include <cstdio>
#include <string>

namespace floating_temple {

class Peer;

namespace toy_lang {

void RunToyLangProgram(Peer* peer, const std::string& source_file_name);
void RunToyLangFile(Peer* peer, std::FILE* fp);


}  // namespace toy_lang
}  // namespace floating_temple

#endif  // TOY_LANG_RUN_TOY_LANG_PROGRAM_H_
