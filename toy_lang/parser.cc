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

#include <memory>
#include <string>
#include <vector>

#include "base/logging.h"
#include "toy_lang/expression.h"
#include "toy_lang/lexer.h"
#include "toy_lang/symbol_table.h"
#include "toy_lang/token.h"

using std::shared_ptr;
using std::string;
using std::vector;

namespace floating_temple {
namespace toy_lang {

Parser::Parser(Lexer* lexer)
    : lexer_(CHECK_NOTNULL(lexer)) {
}

Expression* Parser::ParseFile() {
  vector<Expression*> list_items;

  symbol_table_.EnterScope(vector<string>());

  while (lexer_->HasNextToken()) {
    list_items.push_back(ParseExpression());
  }

  vector<int> parameter_symbol_ids;
  vector<int> local_symbol_ids;
  symbol_table_.LeaveScope(&parameter_symbol_ids, &local_symbol_ids);
  CHECK_EQ(parameter_symbol_ids.size(), 0u);

  const shared_ptr<const Expression> list_expression(
        new ListExpression(list_items));
  Expression* const block_expression = new BlockExpression(list_expression,
                                                           parameter_symbol_ids,
                                                           local_symbol_ids);

  VLOG(2) << block_expression->DebugString();

  return block_expression;
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

    case Token::IDENTIFIER: {
      const int symbol_id = symbol_table_.GetSymbolId(token.identifier());
      return new SymbolExpression(symbol_id);
    }

    case Token::BEGIN_EXPRESSION: {
      Expression* const function = ParseExpression();

      vector<Expression*> parameters;
      ParseExpressionList(Token::END_EXPRESSION, &parameters);

      return new FunctionCallExpression(function, parameters);
    }

    case Token::BEGIN_BLOCK: {
      symbol_table_.EnterScope(vector<string>());

      const shared_ptr<const Expression> expression(ParseExpression());
      CHECK_EQ(lexer_->GetNextTokenType(), Token::END_BLOCK);

      vector<int> parameter_symbol_ids;
      vector<int> local_symbol_ids;
      symbol_table_.LeaveScope(&parameter_symbol_ids, &local_symbol_ids);
      CHECK_EQ(parameter_symbol_ids.size(), 0u);

      return new BlockExpression(expression, parameter_symbol_ids,
                                 local_symbol_ids);
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
