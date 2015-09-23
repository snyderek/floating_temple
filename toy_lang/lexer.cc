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

#include "toy_lang/lexer.h"

#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <queue>
#include <string>

#include "base/escape.h"
#include "base/integral_types.h"
#include "base/logging.h"
#include "base/string_printf.h"
#include "toy_lang/token.h"

using std::FILE;
using std::fgetc;
using std::isdigit;
using std::isgraph;
using std::isspace;
using std::string;
using std::strtoll;

namespace floating_temple {
namespace toy_lang {
namespace {

string Hex(int n) {
  return StringPrintf("%02X", n);
}

}  // namespace

Lexer::Lexer(FILE* fp)
    : fp_(CHECK_NOTNULL(fp)),
      state_(START) {
}

Lexer::~Lexer() {
  FetchTokens();
  CHECK_EQ(tokens_.size(), 0u);
  CHECK_EQ(state_, END_OF_FILE);
}

bool Lexer::HasNextToken() const {
  FetchTokens();
  return !tokens_.empty();
}

void Lexer::GetNextToken(Token* token) {
  FetchTokens();
  CHECK(!tokens_.empty());

  *token = tokens_.front();
  tokens_.pop();
}

Token::Type Lexer::PeekNextTokenType() const {
  FetchTokens();
  CHECK(!tokens_.empty());

  return tokens_.front().type();
}

Token::Type Lexer::GetNextTokenType() {
  FetchTokens();
  CHECK(!tokens_.empty());

  const Token::Type token_type = tokens_.front().type();
  tokens_.pop();

  return token_type;
}

void Lexer::FetchTokens() const {
  while (state_ != END_OF_FILE && tokens_.empty()) {
    // fgetc returns EOF if end-of-file was reached, or if an error occurred. To
    // distinguish between the two cases, we have to set errno to zero before
    // calling fgetc, and then check if errno is non-zero afterward.
    errno = 0;
    const int c = fgetc(fp_);
    PLOG_IF(FATAL, c == EOF && errno != 0) << "fgetc";

    const char char_c = static_cast<char>(static_cast<unsigned char>(c));

    switch (state_) {
      case START:
        if (c == EOF) {
          ChangeState(END_OF_FILE);
        } else if (char_c == '"') {
          ChangeState(STRING_LITERAL);
        } else if (char_c == '#') {
          ChangeState(COMMENT);
        } else if (char_c == '(') {
          YieldBeginExpression(START);
        } else if (char_c == ')') {
          YieldEndExpression(END_OF_EXPRESSION);
        } else if (char_c == '-') {
          ChangeState(MINUS_SIGN);
          attribute_ += '-';
        } else if (char_c == '[') {
          YieldBeginList(START);
        } else if (char_c == ']') {
          YieldEndList(END_OF_EXPRESSION);
        } else if (char_c == '{') {
          YieldBeginBlock(START);
        } else if (char_c == '}') {
          YieldEndBlock(END_OF_EXPRESSION);
        } else if (isspace(c)) {
        } else if (isdigit(c)) {
          ChangeState(INT_LITERAL);
          attribute_ += char_c;
        } else if (isgraph(c)) {
          ChangeState(IDENTIFIER);
          attribute_ += char_c;
        } else {
          LOG(FATAL) << "Unexpected character: '\\x" << Hex(c) << "'";
        }
        break;

      case COMMENT:
        if (c == EOF) {
          ChangeState(END_OF_FILE);
        } else if (char_c == '\n' || char_c == '\r') {
          ChangeState(START);
        }
        break;

      case MINUS_SIGN:
        if (c == EOF) {
          YieldIdentifier(END_OF_FILE);
        } else if (char_c == '#') {
          YieldIdentifier(COMMENT);
        } else if (char_c == ')') {
          YieldIdentifier(END_OF_EXPRESSION);
          YieldEndExpression(END_OF_EXPRESSION);
        } else if (char_c == ']') {
          YieldIdentifier(END_OF_EXPRESSION);
          YieldEndList(END_OF_EXPRESSION);
        } else if (char_c == '}') {
          YieldIdentifier(END_OF_EXPRESSION);
          YieldEndBlock(END_OF_EXPRESSION);
        } else if (isspace(c)) {
          YieldIdentifier(START);
        } else if (isdigit(c)) {
          ChangeState(INT_LITERAL);
          attribute_ += '-';
          attribute_ += char_c;
        } else if (isgraph(c) && char_c != '"' && char_c != '(' &&
                   char_c != '[' && char_c != '{') {
          ChangeState(IDENTIFIER);
          attribute_ += '-';
          attribute_ += char_c;
        } else {
          LOG(FATAL) << "Unexpected character: '\\x" << Hex(c) << "'";
        }
        break;

      case INT_LITERAL:
        if (c == EOF) {
          YieldIntLiteral(END_OF_FILE);
        } else if (char_c == '#') {
          YieldIntLiteral(COMMENT);
        } else if (char_c == ')') {
          YieldIntLiteral(END_OF_EXPRESSION);
          YieldEndExpression(END_OF_EXPRESSION);
        } else if (char_c == ']') {
          YieldIntLiteral(END_OF_EXPRESSION);
          YieldEndList(END_OF_EXPRESSION);
        } else if (char_c == '}') {
          YieldIntLiteral(END_OF_EXPRESSION);
          YieldEndBlock(END_OF_EXPRESSION);
        } else if (isspace(c)) {
          YieldIntLiteral(START);
        } else if (isdigit(c)) {
          attribute_ += char_c;
        } else {
          LOG(FATAL) << "Unexpected character: '\\x" << Hex(c) << "'";
        }
        break;

      case STRING_LITERAL:
        // TODO(dss): Add support for escape sequences inside string literals.
        if (c == EOF) {
          LOG(FATAL) << "End of file detected while processing string literal.";
        } else if (char_c == '"') {
          YieldStringLiteral(END_OF_EXPRESSION);
        } else {
          attribute_ += char_c;
        }
        break;

      case IDENTIFIER:
        if (c == EOF) {
          YieldIdentifier(END_OF_FILE);
        } else if (char_c == '#') {
          YieldIdentifier(COMMENT);
        } else if (char_c == ')') {
          YieldIdentifier(END_OF_EXPRESSION);
          YieldEndExpression(END_OF_EXPRESSION);
        } else if (char_c == ']') {
          YieldIdentifier(END_OF_EXPRESSION);
          YieldEndList(END_OF_EXPRESSION);
        } else if (char_c == '}') {
          YieldIdentifier(END_OF_EXPRESSION);
          YieldEndBlock(END_OF_EXPRESSION);
        } else if (isspace(c)) {
          YieldIdentifier(START);
        } else if (isgraph(c) && char_c != '"' && char_c != '(' &&
                   char_c != '[' && char_c != '{') {
          attribute_+= char_c;
        } else {
          LOG(FATAL) << "Unexpected character: '\\x" << Hex(c) << "'";
        }
        break;

      case END_OF_EXPRESSION:
        if (c == EOF) {
          ChangeState(END_OF_FILE);
        } else if (char_c == '#') {
          ChangeState(COMMENT);
        } else if (char_c == ')') {
          YieldEndExpression(END_OF_EXPRESSION);
        } else if (char_c == ']') {
          YieldEndList(END_OF_EXPRESSION);
        } else if (char_c == '}') {
          YieldEndBlock(END_OF_EXPRESSION);
        } else if (isspace(c)) {
          ChangeState(START);
        } else {
          LOG(FATAL) << "Unexpected character: '\\x" << Hex(c) << "'";
        }
        break;

      default:
        LOG(FATAL) << "Unexpected state: " << static_cast<int>(state_);
    }
  }
}

void Lexer::YieldIntLiteral(State new_state) const {
  CHECK(!attribute_.empty());

  char* endptr = nullptr;
  errno = 0;
  const long long n = strtoll(attribute_.c_str(), &endptr, 10);
  if (n == LLONG_MIN && errno == ERANGE) {
    LOG(FATAL) << "Underflow in integer literal: \"" << CEscape(attribute_)
               << "\"";
  }
  if (n == LLONG_MAX && errno == ERANGE) {
    LOG(FATAL) << "Overflow in integer literal: \"" << CEscape(attribute_)
               << "\"";
  }
  if (*endptr != '\0') {
    LOG(FATAL) << "Integer literal is invalid: \"" << CEscape(attribute_)
               << "\"";
  }

  tokens_.push(Token());
  Token::CreateIntLiteral(&tokens_.back(), static_cast<int64>(n));
  ChangeState(new_state);
}

void Lexer::YieldStringLiteral(State new_state) const {
  tokens_.push(Token());
  Token::CreateStringLiteral(&tokens_.back(), attribute_);
  ChangeState(new_state);
}

void Lexer::YieldIdentifier(State new_state) const {
  tokens_.push(Token());
  Token::CreateIdentifier(&tokens_.back(), attribute_);
  ChangeState(new_state);
}

void Lexer::YieldBeginExpression(State new_state) const {
  tokens_.push(Token());
  Token::CreateBeginExpression(&tokens_.back());
  ChangeState(new_state);
}

void Lexer::YieldEndExpression(State new_state) const {
  tokens_.push(Token());
  Token::CreateEndExpression(&tokens_.back());
  ChangeState(new_state);
}

void Lexer::YieldBeginBlock(State new_state) const {
  tokens_.push(Token());
  Token::CreateBeginBlock(&tokens_.back());
  ChangeState(new_state);
}

void Lexer::YieldEndBlock(State new_state) const {
  tokens_.push(Token());
  Token::CreateEndBlock(&tokens_.back());
  ChangeState(new_state);
}

void Lexer::YieldBeginList(State new_state) const {
  tokens_.push(Token());
  Token::CreateBeginList(&tokens_.back());
  ChangeState(new_state);
}

void Lexer::YieldEndList(State new_state) const {
  tokens_.push(Token());
  Token::CreateEndList(&tokens_.back());
  ChangeState(new_state);
}

void Lexer::ChangeState(State new_state) const {
  if (state_ != new_state) {
    state_ = new_state;
    attribute_.clear();
  }
}

}  // namespace toy_lang
}  // namespace floating_temple
