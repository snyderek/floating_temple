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

#include "toy_lang/token.h"

#include <string>

#include "base/escape.h"
#include "base/integral_types.h"
#include "base/logging.h"

using std::string;

namespace floating_temple {
namespace toy_lang {
namespace {

bool IsIntAttributeType(Token::Type token_type) {
  return token_type == Token::INT_LITERAL;
}

bool IsStringAttributeType(Token::Type token_type) {
  return token_type == Token::STRING_LITERAL ||
      token_type == Token::SYMBOL_LITERAL || token_type == Token::IDENTIFIER;
}

}  // namespace

Token::Token()
    : type_(UNINITIALIZED) {
}

Token::Token(const Token& other)
    : type_(other.type_) {
  if (IsIntAttributeType(type_)) {
    int_attribute_ = other.int_attribute_;
  }
  if (IsStringAttributeType(type_)) {
    string_attribute_ = new string(*other.string_attribute_);
  }
}

Token::~Token() {
  FreeMemory();
}

int64 Token::int_literal() const {
  CHECK_EQ(type_, INT_LITERAL);
  return int_attribute_;
}

const string& Token::string_literal() const {
  CHECK_EQ(type_, STRING_LITERAL);
  return *string_attribute_;
}

const string& Token::symbol_name() const {
  CHECK_EQ(type_, SYMBOL_LITERAL);
  return *string_attribute_;
}

const string& Token::identifier() const {
  CHECK_EQ(type_, IDENTIFIER);
  return *string_attribute_;
}

Token& Token::operator=(const Token& other) {
  if (IsStringAttributeType(type_)) {
    if (!IsStringAttributeType(other.type_)) {
      delete string_attribute_;
    }
  } else {
    if (IsStringAttributeType(other.type_)) {
      string_attribute_ = new string();
    }
  }

  type_ = other.type_;

  if (IsIntAttributeType(type_)) {
    int_attribute_ = other.int_attribute_;
  }
  if (IsStringAttributeType(type_)) {
    *string_attribute_ = *other.string_attribute_;
  }

  return *this;
}

// static
void Token::CreateIntLiteral(Token* token, int64 int_literal) {
  CHECK(token != nullptr);
  VLOG(1) << "INT_LITERAL " << int_literal;
  token->FreeMemory();
  token->type_ = INT_LITERAL;
  token->int_attribute_ = int_literal;
}

// static
void Token::CreateStringLiteral(Token* token, const string& string_literal) {
  CHECK(token != nullptr);
  VLOG(1) << "STRING_LITERAL \"" << CEscape(string_literal) << "\"";
  token->FreeMemory();
  token->type_ = STRING_LITERAL;
  token->string_attribute_ = new string(string_literal);
}

// static
void Token::CreateSymbolLiteral(Token* token, const string& symbol_name) {
  CHECK(token != nullptr);
  VLOG(1) << "SYMBOL_LITERAL \"" << CEscape(symbol_name) << "\"";
  token->FreeMemory();
  token->type_ = SYMBOL_LITERAL;
  token->string_attribute_ = new string(symbol_name);
}

// static
void Token::CreateIdentifier(Token* token, const string& identifier) {
  CHECK(token != nullptr);
  VLOG(1) << "IDENTIFIER \"" << CEscape(identifier) << "\"";
  token->FreeMemory();
  token->type_ = IDENTIFIER;
  token->string_attribute_ = new string(identifier);
}

// static
void Token::CreateBeginExpression(Token* token) {
  CHECK(token != nullptr);
  VLOG(1) << "BEGIN_EXPRESSION";
  token->FreeMemory();
  token->type_ = BEGIN_EXPRESSION;
}

// static
void Token::CreateEndExpression(Token* token) {
  CHECK(token != nullptr);
  VLOG(1) << "END_EXPRESSION";
  token->FreeMemory();
  token->type_ = END_EXPRESSION;
}

// static
void Token::CreateBeginBlock(Token* token) {
  CHECK(token != nullptr);
  VLOG(1) << "BEGIN_BLOCK";
  token->FreeMemory();
  token->type_ = BEGIN_BLOCK;
}

// static
void Token::CreateEndBlock(Token* token) {
  CHECK(token != nullptr);
  VLOG(1) << "END_BLOCK";
  token->FreeMemory();
  token->type_ = END_BLOCK;
}

// static
void Token::CreateBeginList(Token* token) {
  CHECK(token != nullptr);
  VLOG(1) << "BEGIN_LIST";
  token->FreeMemory();
  token->type_ = BEGIN_LIST;
}

// static
void Token::CreateEndList(Token* token) {
  CHECK(token != nullptr);
  VLOG(1) << "END_LIST";
  token->FreeMemory();
  token->type_ = END_LIST;
}

void Token::FreeMemory() {
  if (IsStringAttributeType(type_)) {
    delete string_attribute_;
  }
}

bool operator==(const Token& lhs, const Token& rhs) {
  const Token::Type lhs_type = lhs.type();
  const Token::Type rhs_type = rhs.type();

  if (lhs_type != rhs_type) {
    return false;
  }

  if (IsIntAttributeType(lhs_type)) {
    if (lhs.int_attribute_ != rhs.int_attribute_) {
      return false;
    }
  }
  if (IsStringAttributeType(lhs_type)) {
    if (*lhs.string_attribute_ != *rhs.string_attribute_) {
      return false;
    }
  }

  return true;
}

bool operator!=(const Token& lhs, const Token& rhs) {
  return !(lhs == rhs);
}

}  // namespace toy_lang
}  // namespace floating_temple
