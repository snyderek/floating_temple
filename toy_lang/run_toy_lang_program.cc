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

#include "toy_lang/run_toy_lang_program.h"

#include <cstdio>
#include <memory>
#include <string>

#include "base/logging.h"
#include "include/c++/peer.h"
#include "include/c++/value.h"
#include "toy_lang/expression.h"
#include "toy_lang/lexer.h"
#include "toy_lang/parser.h"
#include "toy_lang/program_object.h"
#include "toy_lang/symbol_table.h"

using std::FILE;
using std::fclose;
using std::fopen;
using std::shared_ptr;
using std::string;

namespace floating_temple {
namespace toy_lang {

void RunToyLangProgram(Peer* peer, const string& source_file_name,
                       bool linger) {
  // Run the source file.
  FILE* const fp = fopen(source_file_name.c_str(), "r");
  PLOG_IF(FATAL, fp == nullptr) << "fopen";

  RunToyLangFile(peer, fp, linger);

  PLOG_IF(FATAL, fclose(fp) != 0) << "fclose";
}

void RunToyLangFile(Peer* peer, FILE* fp, bool linger) {
  CHECK(peer != nullptr);

  Lexer lexer(fp);
  SymbolTable symbol_table;
  Parser parser(&lexer, &symbol_table);
  const shared_ptr<const Expression> expression(parser.ParseFile());

  Value return_value;
  peer->RunProgram(new ProgramObject(expression), "run", &return_value, linger);
}

}  // namespace toy_lang
}  // namespace floating_temple
