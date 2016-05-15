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

#include "toy_lang/parser.h"

#include <vector>

#include "base/logging.h"
#include "toy_lang/expression.h"
#include "toy_lang/lexer.h"
#include "toy_lang/token.h"

using std::vector;

namespace floating_temple {
namespace toy_lang {

Parser::Parser(Lexer* lexer)
    : lexer_(CHECK_NOTNULL(lexer)) {
}

Expression* Parser::ParseFile() {
  Expression* const expression = ParseScope();
  CHECK(!lexer_->HasNextToken());

  VLOG(2) << expression->DebugString();

  return expression;
}

Expression* Parser::ParseScope() {
  Expression* const expression = ParseExpression();

  return expression;
}

Expression* Parser::ParseExpression() {
  Token token;
  lexer_->GetNextToken(&token);
  const Token::Type token_type = token.type();

  switch (token_type) {
    case Token::INT_LITERAL:
      return new IntExpression(token.int_literal());

    case Token::STRING_LITERAL:
      return new StringExpression(token.string_literal());

    case Token::IDENTIFIER:
      // TODO(dss): Implement this.
      break;

    case Token::BEGIN_EXPRESSION: {
      Expression* const function = ParseExpression();

      vector<Expression*> parameters;
      ParseExpressionList(Token::END_EXPRESSION, &parameters);

      return new FunctionCallExpression(function, parameters);
    }

    case Token::BEGIN_BLOCK: {
      Expression* const expression = ParseScope();
      CHECK_EQ(lexer_->GetNextTokenType(), Token::END_BLOCK);

      // TODO(dss): Set the 'bound_symbol_count' and 'unbound_symbol_count'
      // parameters.
      return new BlockExpression(expression, 0, 0);
    }

    case Token::BEGIN_LIST: {
      vector<Expression*> list_items;
      ParseExpressionList(Token::END_LIST, &list_items);

      return new ListExpression(list_items);
    }

    default:
      LOG(FATAL) << "Unexpected token type: " << static_cast<int>(token_type);
  }

  return nullptr;
}

void Parser::ParseExpressionList(Token::Type end_token_type,
                                 vector<Expression*>* expressions) {
  CHECK(expressions != nullptr);

  while (lexer_->PeekNextTokenType() != end_token_type) {
    expressions->push_back(ParseExpression());
  }

  CHECK_EQ(lexer_->GetNextTokenType(), end_token_type);
}

}  // namespace toy_lang
}  // namespace floating_temple
