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
#include <unordered_map>

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
using std::unordered_map;

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

  SymbolTable symbol_table;

  symbol_table.AddExternalSymbol("get", false);
  symbol_table.AddExternalSymbol("set", false);
  symbol_table.AddExternalSymbol("for", false);
  symbol_table.AddExternalSymbol("while", false);

  symbol_table.AddExternalSymbol("false", true);
  symbol_table.AddExternalSymbol("true", true);
  symbol_table.AddExternalSymbol("list", true);
  symbol_table.AddExternalSymbol("range", true);
  symbol_table.AddExternalSymbol("print", true);
  symbol_table.AddExternalSymbol("add", true);
  symbol_table.AddExternalSymbol("begin_tran", true);
  symbol_table.AddExternalSymbol("end_tran", true);
  symbol_table.AddExternalSymbol("if", true);
  symbol_table.AddExternalSymbol("not", true);
  symbol_table.AddExternalSymbol("lt", true);
  symbol_table.AddExternalSymbol("len", true);
  symbol_table.AddExternalSymbol("list.append", true);
  symbol_table.AddExternalSymbol("list.get", true);
  symbol_table.AddExternalSymbol("map.is_set", true);
  symbol_table.AddExternalSymbol("map.get", true);
  symbol_table.AddExternalSymbol("map.set", true);

  symbol_table.AddExternalSymbol("shared", true);

  Lexer lexer(fp);
  Parser parser(&lexer, &symbol_table);
  const shared_ptr<const Expression> expression(parser.ParseFile());

  unordered_map<string, int> external_symbol_ids;
  symbol_table.GetExternalSymbolIds(&external_symbol_ids);

  LocalObject* const program_object = new ProgramObject(external_symbol_ids,
                                                        expression);

  Value return_value;
  peer->RunProgram(program_object, "run", &return_value, linger);
  CHECK_EQ(return_value.type(), Value::EMPTY);
}

}  // namespace toy_lang
}  // namespace floating_temple
