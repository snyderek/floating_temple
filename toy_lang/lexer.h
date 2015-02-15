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

#ifndef TOY_LANG_LEXER_H_
#define TOY_LANG_LEXER_H_

#include <cstdio>
#include <queue>
#include <string>

#include "base/macros.h"
#include "toy_lang/token.h"

namespace floating_temple {
namespace toy_lang {

class Lexer {
 public:
  explicit Lexer(std::FILE* fp);
  ~Lexer();

  bool HasNextToken() const;
  void GetNextToken(Token* token);
  Token::Type PeekNextTokenType() const;
  Token::Type GetNextTokenType();

 private:
  enum State {
    START,
    COMMENT,
    MINUS_SIGN,
    INT_LITERAL,
    STRING_LITERAL,
    IDENTIFIER,
    END_OF_EXPRESSION,
    END_OF_FILE
  };

  void FetchTokens() const;

  void YieldIntLiteral(State new_state) const;
  void YieldStringLiteral(State new_state) const;
  void YieldIdentifier(State new_state) const;
  void YieldBeginExpression(State new_state) const;
  void YieldEndExpression(State new_state) const;
  void YieldBeginBlock(State new_state) const;
  void YieldEndBlock(State new_state) const;
  void YieldBeginList(State new_state) const;
  void YieldEndList(State new_state) const;

  void ChangeState(State new_state) const;

  std::FILE* const fp_;
  mutable std::queue<Token> tokens_;
  mutable State state_;
  mutable std::string attribute_;

  DISALLOW_COPY_AND_ASSIGN(Lexer);
};

}  // namespace toy_lang
}  // namespace floating_temple

#endif  // TOY_LANG_LEXER_H_
