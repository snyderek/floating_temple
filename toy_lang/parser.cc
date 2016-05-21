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
#include "toy_lang/hidden_symbols.h"
#include "toy_lang/lexer.h"
#include "toy_lang/symbol_table.h"
#include "toy_lang/token.h"

using std::shared_ptr;
using std::string;
using std::vector;

namespace floating_temple {
namespace toy_lang {

Parser::Parser(Lexer* lexer, SymbolTable* symbol_table,
               const HiddenSymbols& hidden_symbols)
    : lexer_(CHECK_NOTNULL(lexer)),
      symbol_table_(CHECK_NOTNULL(symbol_table)),
      hidden_symbols_(hidden_symbols) {
}

Expression* Parser::ParseFile() {
  EnterScope(vector<string>());

  vector<Expression*> list_items;
  while (lexer_->HasNextToken()) {
    list_items.push_back(ParseExpression());
  }

  const shared_ptr<const Expression> list_expression(
        new ListExpression(list_items));
  Expression* const block_expression = LeaveScope(list_expression);

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
      Expression* const function_expression = new SymbolExpression(
          hidden_symbols_.get_variable_symbol_id);

      const int symbol_id = symbol_table_->GetSymbolId(token.identifier());
      Expression* const variable_expression = new SymbolExpression(symbol_id);
      const vector<Expression*> parameters(1, variable_expression);

      return new FunctionCallExpression(function_expression, parameters);
    }

    case Token::BEGIN_EXPRESSION:
      if (lexer_->PeekNextTokenType() == Token::SET_KEYWORD) {
        CHECK_EQ(lexer_->GetNextTokenType(), Token::SET_KEYWORD);

        Token identifier_token;
        lexer_->GetNextToken(&identifier_token);
        const string& identifier = identifier_token.identifier();

        Expression* const rhs_expression = ParseExpression();
        CHECK_EQ(lexer_->GetNextTokenType(), Token::END_EXPRESSION);

        const int symbol_id = symbol_table_->GetSymbolId(identifier);
        Expression* const variable_expression = new SymbolExpression(symbol_id);

        Expression* const function_expression = new SymbolExpression(
            hidden_symbols_.set_variable_symbol_id);

        vector<Expression*> parameters(2);
        parameters[0] = variable_expression;
        parameters[1] = rhs_expression;

        return new FunctionCallExpression(function_expression, parameters);
      } else {
        Expression* const function = ParseExpression();
        vector<Expression*> parameters;
        ParseExpressionList(Token::END_EXPRESSION, &parameters);

        return new FunctionCallExpression(function, parameters);
      }
      break;

    case Token::BEGIN_BLOCK: {
      EnterScope(vector<string>());

      const shared_ptr<const Expression> expression(ParseExpression());
      CHECK_EQ(lexer_->GetNextTokenType(), Token::END_BLOCK);

      return LeaveScope(expression);
    }

    case Token::BEGIN_LIST: {
      vector<Expression*> list_items;
      ParseExpressionList(Token::END_LIST, &list_items);

      return new ListExpression(list_items);
    }

    default:
      LOG(FATAL) << "Unexpected token type: " << static_cast<int>(token_type);
  }

  LOG(FATAL) << "Execution should not reach this point.";
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

void Parser::EnterScope(const vector<string>& parameter_names) {
  symbol_table_->EnterScope(parameter_names);
}

Expression* Parser::LeaveScope(const shared_ptr<const Expression>& expression) {
  vector<int> parameter_symbol_ids;
  vector<int> local_symbol_ids;
  symbol_table_->LeaveScope(&parameter_symbol_ids, &local_symbol_ids);

  return new BlockExpression(expression, parameter_symbol_ids,
                             local_symbol_ids);
}

}  // namespace toy_lang
}  // namespace floating_temple
