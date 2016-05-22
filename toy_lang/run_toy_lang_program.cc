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
#include "toy_lang/zoo/add_function.h"
#include "toy_lang/zoo/begin_tran_function.h"
#include "toy_lang/zoo/bool_object.h"
#include "toy_lang/zoo/end_tran_function.h"
#include "toy_lang/zoo/for_function.h"
#include "toy_lang/zoo/get_variable_function.h"
#include "toy_lang/zoo/if_function.h"
#include "toy_lang/zoo/len_function.h"
#include "toy_lang/zoo/less_than_function.h"
#include "toy_lang/zoo/list_append_function.h"
#include "toy_lang/zoo/list_function.h"
#include "toy_lang/zoo/list_get_function.h"
#include "toy_lang/zoo/map_get_function.h"
#include "toy_lang/zoo/map_is_set_function.h"
#include "toy_lang/zoo/map_object.h"
#include "toy_lang/zoo/map_set_function.h"
#include "toy_lang/zoo/not_function.h"
#include "toy_lang/zoo/print_function.h"
#include "toy_lang/zoo/range_function.h"
#include "toy_lang/zoo/set_variable_function.h"
#include "toy_lang/zoo/while_function.h"

using std::FILE;
using std::fclose;
using std::fopen;
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

  SymbolTable symbol_table;

  symbol_table.AddExternalSymbol("get", false, new GetVariableFunction());
  symbol_table.AddExternalSymbol("set", false, new SetVariableFunction());
  symbol_table.AddExternalSymbol("for", false, new ForFunction());
  symbol_table.AddExternalSymbol("while", false, new WhileFunction());

  // TODO(dss): Make these unversioned objects. There's no reason to record
  // method calls on any of these objects, because they're constant. (The
  // "shared" map object should still be versioned, however.)
  symbol_table.AddExternalSymbol("false", true, new BoolObject(false));
  symbol_table.AddExternalSymbol("true", true, new BoolObject(true));
  symbol_table.AddExternalSymbol("list", true, new ListFunction());
  symbol_table.AddExternalSymbol("range", true, new RangeFunction());
  symbol_table.AddExternalSymbol("print", true, new PrintFunction());
  symbol_table.AddExternalSymbol("add", true, new AddFunction());
  symbol_table.AddExternalSymbol("begin_tran", true, new BeginTranFunction());
  symbol_table.AddExternalSymbol("end_tran", true, new EndTranFunction());
  symbol_table.AddExternalSymbol("if", true, new IfFunction());
  symbol_table.AddExternalSymbol("not", true, new NotFunction());
  symbol_table.AddExternalSymbol("lt", true, new LessThanFunction());
  symbol_table.AddExternalSymbol("len", true, new LenFunction());
  symbol_table.AddExternalSymbol("list.append", true, new ListAppendFunction());
  symbol_table.AddExternalSymbol("list.get", true, new ListGetFunction());
  symbol_table.AddExternalSymbol("map.is_set", true, new MapIsSetFunction());
  symbol_table.AddExternalSymbol("map.get", true, new MapGetFunction());
  symbol_table.AddExternalSymbol("map.set", true, new MapSetFunction());

  symbol_table.AddExternalSymbol("shared", true, new MapObject());

  Lexer lexer(fp);
  Parser parser(&lexer, &symbol_table);
  Expression* const expression = parser.ParseFile();

  UnversionedLocalObject* const program_object = new ProgramObject(
      &symbol_table, expression);

  Value return_value;
  peer->RunProgram(program_object, "run", &return_value, linger);
  CHECK_EQ(return_value.type(), Value::EMPTY);
}

}  // namespace toy_lang
}  // namespace floating_temple
