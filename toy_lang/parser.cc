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

Parser::Parser(Lexer* lexer, SymbolTable* symbol_table)
    : lexer_(CHECK_NOTNULL(lexer)),
      symbol_table_(CHECK_NOTNULL(symbol_table)) {
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
  const Token::Type token_type = lexer_->PeekNextTokenType();
  switch (token_type) {
    case Token::INT_LITERAL:
      return ParseIntLiteral();

    case Token::STRING_LITERAL:
      return ParseStringLiteral();

    case Token::IDENTIFIER:
      return ParseIdentifier();

    case Token::BEGIN_EXPRESSION:
      return ParseStatement();

    case Token::BEGIN_BLOCK:
      return ParseBlock(vector<string>());

    case Token::BEGIN_LIST:
      return ParseList();

    default:
      LOG(FATAL) << "Unexpected token type: " << static_cast<int>(token_type);
  }
}

Expression* Parser::ParseIntLiteral() {
  Token token;
  lexer_->GetNextToken(&token);
  return new IntExpression(token.int_literal());
}

Expression* Parser::ParseStringLiteral() {
  Token token;
  lexer_->GetNextToken(&token);
  return new StringExpression(token.string_literal());
}

Expression* Parser::ParseIdentifier() {
  Token token;
  lexer_->GetNextToken(&token);

  Expression* const function_expression = new SymbolExpression(
      symbol_table_->GetSymbolId("get", false));

  const int symbol_id = symbol_table_->GetSymbolId(token.identifier(),
                                                   true);
  Expression* const variable_expression = new SymbolExpression(symbol_id);
  const vector<Expression*> parameters(1, variable_expression);

  return new FunctionCallExpression(function_expression, parameters);
}

Expression* Parser::ParseStatement() {
  CHECK_EQ(lexer_->GetNextTokenType(), Token::BEGIN_EXPRESSION);

  Expression* expression = nullptr;
  switch (lexer_->PeekNextTokenType()) {
    case Token::FOR_KEYWORD:
      expression = ParseForStatement();
      break;

    case Token::SET_KEYWORD:
      expression = ParseSetStatement();
      break;

    case Token::WHILE_KEYWORD:
      expression = ParseWhileStatement();
      break;

    default:
      expression = ParseFunctionCall();
  }

  CHECK_EQ(lexer_->GetNextTokenType(), Token::END_EXPRESSION);

  return expression;
}

Expression* Parser::ParseForStatement() {
  CHECK_EQ(lexer_->GetNextTokenType(), Token::FOR_KEYWORD);

  Token identifier_token;
  lexer_->GetNextToken(&identifier_token);
  const string& identifier = identifier_token.identifier();

  Expression* const range_expression = ParseExpression();
  Expression* const block_expression = ParseBlock(
      vector<string>(1, identifier));

  Expression* const function_expression = new SymbolExpression(
      symbol_table_->GetSymbolId("for", false));

  vector<Expression*> parameters(2);
  parameters[0] = range_expression;
  parameters[1] = block_expression;

  return new FunctionCallExpression(function_expression, parameters);
}

Expression* Parser::ParseSetStatement() {
  CHECK_EQ(lexer_->GetNextTokenType(), Token::SET_KEYWORD);

  Token identifier_token;
  lexer_->GetNextToken(&identifier_token);
  const string& identifier = identifier_token.identifier();

  Expression* const rhs_expression = ParseExpression();

  const int symbol_id = symbol_table_->GetLocalVariable(identifier);
  Expression* const variable_expression = new SymbolExpression(
      symbol_id);

  Expression* const function_expression = new SymbolExpression(
      symbol_table_->GetSymbolId("set", false));

  vector<Expression*> parameters(2);
  parameters[0] = variable_expression;
  parameters[1] = rhs_expression;

  return new FunctionCallExpression(function_expression, parameters);
}

Expression* Parser::ParseWhileStatement() {
  CHECK_EQ(lexer_->GetNextTokenType(), Token::WHILE_KEYWORD);

  EnterScope(vector<string>());
  const shared_ptr<const Expression> condition_expression(ParseExpression());
  Expression* const condition_block_expression = LeaveScope(
      condition_expression);

  Expression* const code_block_expression = ParseBlock(vector<string>());

  Expression* const function_expression = new SymbolExpression(
      symbol_table_->GetSymbolId("while", false));

  vector<Expression*> parameters(2);
  parameters[0] = condition_block_expression;
  parameters[1] = code_block_expression;

  return new FunctionCallExpression(function_expression, parameters);
}

Expression* Parser::ParseFunctionCall() {
  Expression* const function = ParseExpression();

  vector<Expression*> parameters;
  ParseExpressionList(Token::END_EXPRESSION, &parameters);

  return new FunctionCallExpression(function, parameters);
}

Expression* Parser::ParseBlock(const vector<string>& parameter_names) {
  CHECK_EQ(lexer_->GetNextTokenType(), Token::BEGIN_BLOCK);

  EnterScope(parameter_names);
  vector<Expression*> expressions;
  ParseExpressionList(Token::END_BLOCK, &expressions);
  const shared_ptr<const Expression> list_expression(
      new ListExpression(expressions));
  Expression* const block_expression = LeaveScope(list_expression);

  CHECK_EQ(lexer_->GetNextTokenType(), Token::END_BLOCK);

  return block_expression;
}

Expression* Parser::ParseList() {
  CHECK_EQ(lexer_->GetNextTokenType(), Token::BEGIN_LIST);
  vector<Expression*> list_items;
  ParseExpressionList(Token::END_LIST, &list_items);
  CHECK_EQ(lexer_->GetNextTokenType(), Token::END_LIST);

  return new ListExpression(list_items);
}

void Parser::ParseExpressionList(Token::Type end_token_type,
                                 vector<Expression*>* expressions) {
  CHECK(expressions != nullptr);

  while (lexer_->PeekNextTokenType() != end_token_type) {
    expressions->push_back(ParseExpression());
  }
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
