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
namespace {

void ParseExpressionList(Lexer* lexer, Token::Type end_token_type,
                         vector<Expression*>* expressions);

Expression* ParseExpression(Lexer* lexer) {
  CHECK(lexer != nullptr);

  Token token;
  lexer->GetNextToken(&token);
  const Token::Type token_type = token.type();

  switch (token_type) {
    case Token::INT_LITERAL:
      return new IntExpression(token.int_literal());

    case Token::STRING_LITERAL:
      return new StringExpression(token.string_literal());

    case Token::IDENTIFIER:
      return new VariableExpression(token.identifier());

    case Token::BEGIN_EXPRESSION: {
      Expression* const function = ParseExpression(lexer);

      vector<Expression*> parameters;
      ParseExpressionList(lexer, Token::END_EXPRESSION, &parameters);

      return new FunctionExpression(function, parameters);
    }

    case Token::BEGIN_BLOCK: {
      Expression* const expression = ParseExpression(lexer);
      CHECK_EQ(lexer->GetNextTokenType(), Token::END_BLOCK);

      return new ExpressionExpression(expression);
    }

    case Token::BEGIN_LIST: {
      vector<Expression*> list_items;
      ParseExpressionList(lexer, Token::END_LIST, &list_items);

      return new ListExpression(list_items);
    }

    default:
      LOG(FATAL) << "Unexpected token type: " << static_cast<int>(token_type);
  }

  return nullptr;
}

void ParseExpressionList(Lexer* lexer, Token::Type end_token_type,
                         vector<Expression*>* expressions) {
  CHECK(lexer != nullptr);
  CHECK(expressions != nullptr);

  while (lexer->PeekNextTokenType() != end_token_type) {
    expressions->push_back(ParseExpression(lexer));
  }

  CHECK_EQ(lexer->GetNextTokenType(), end_token_type);
}

}  // namespace

Expression* ParseFile(Lexer* lexer) {
  CHECK(lexer != nullptr);

  Expression* const expression = ParseExpression(lexer);
  CHECK(!lexer->HasNextToken());

  VLOG(2) << expression->DebugString();

  return expression;
}

}  // namespace toy_lang
}  // namespace floating_temple
