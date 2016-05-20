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

#ifndef TOY_LANG_PARSER_H_
#define TOY_LANG_PARSER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "toy_lang/symbol_table.h"
#include "toy_lang/token.h"

namespace floating_temple {
namespace toy_lang {

class Expression;
class Lexer;

class Parser {
 public:
  explicit Parser(Lexer* lexer);

  Expression* ParseFile();

 private:
  Expression* ParseExpression();
  void ParseExpressionList(Token::Type end_token_type,
                           std::vector<Expression*>* expressions);

  void EnterScope(const std::vector<std::string>& parameter_names);
  Expression* LeaveScope(const std::shared_ptr<const Expression>& expression);

  Lexer* const lexer_;
  SymbolTable symbol_table_;

  DISALLOW_COPY_AND_ASSIGN(Parser);
};

}  // namespace toy_lang
}  // namespace floating_temple

#endif  // TOY_LANG_PARSER_H_
