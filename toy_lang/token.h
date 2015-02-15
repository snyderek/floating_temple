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

#ifndef TOY_LANG_TOKEN_H_
#define TOY_LANG_TOKEN_H_

#include <string>

#include "base/integral_types.h"

namespace floating_temple {
namespace toy_lang {

class Token {
 public:
  enum Type {
    UNINITIALIZED,
    INT_LITERAL,
    STRING_LITERAL,
    IDENTIFIER,
    BEGIN_EXPRESSION,
    END_EXPRESSION,
    BEGIN_BLOCK,
    END_BLOCK,
    BEGIN_LIST,
    END_LIST
  };

  Token();
  Token(const Token& other);
  ~Token();

  Type type() const { return type_; }

  int64 int_literal() const;
  const std::string& string_literal() const;
  const std::string& identifier() const;

  Token& operator=(const Token& other);

  static void CreateIntLiteral(Token* token, int64 int_literal);
  static void CreateStringLiteral(Token* token,
                                  const std::string& string_literal);
  static void CreateIdentifier(Token* token, const std::string& identifier);
  static void CreateBeginExpression(Token* token);
  static void CreateEndExpression(Token* token);
  static void CreateBeginBlock(Token* token);
  static void CreateEndBlock(Token* token);
  static void CreateBeginList(Token* token);
  static void CreateEndList(Token* token);

 private:
  void FreeMemory();

  Type type_;

  union {
    int64 int_attribute_;
    std::string* string_attribute_;
  };
};

bool operator==(const Token& lhs, const Token& rhs);
bool operator!=(const Token& lhs, const Token& rhs);

}  // namespace toy_lang
}  // namespace floating_temple

#endif  // TOY_LANG_TOKEN_H_
