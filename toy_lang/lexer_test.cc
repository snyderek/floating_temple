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

#include <cstddef>
#include <cstdio>
#include <string>

#include <gflags/gflags.h>

#include "base/logging.h"
#include "base/macros.h"
#include "third_party/gmock-1.7.0/gtest/include/gtest/gtest.h"
#include "toy_lang/token.h"
#include "util/string_util.h"

using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
using std::FILE;
using std::fclose;
using std::string;
using testing::InitGoogleTest;

namespace floating_temple {
namespace toy_lang {
namespace {

class MemFile {
 public:
  explicit MemFile(const string& file_content)
      : buffer_(CreateCharBuffer(file_content)),
        fp_(fmemopen(buffer_, file_content.length(), "r")) {
    PLOG_IF(FATAL, fp_ == NULL) << "fmemopen";
  }

  ~MemFile() {
    PLOG_IF(FATAL, fclose(fp_) != 0) << "fclose";
    delete[] buffer_;
  }

  FILE* fp() {
    return fp_;
  }

 private:
  char* const buffer_;
  FILE* const fp_;

  DISALLOW_COPY_AND_ASSIGN(MemFile);
};

TEST(LexerTest, NegativeIntegerLiteral) {
  MemFile mem_file("-5");
  Lexer lexer(mem_file.fp());

  Token token;
  lexer.GetNextToken(&token);
  EXPECT_EQ(Token::INT_LITERAL, token.type());
  EXPECT_EQ(-5, token.int_literal());

  EXPECT_FALSE(lexer.HasNextToken());
}

TEST(LexerTest, Hyphen) {
  MemFile mem_file("-");
  Lexer lexer(mem_file.fp());

  Token token;
  lexer.GetNextToken(&token);
  EXPECT_EQ(Token::IDENTIFIER, token.type());
  EXPECT_EQ("-", token.identifier());

  EXPECT_FALSE(lexer.HasNextToken());
}

TEST(LexerTest, DoubleHyphen) {
  MemFile mem_file("--");
  Lexer lexer(mem_file.fp());

  Token token;
  lexer.GetNextToken(&token);
  EXPECT_EQ(Token::IDENTIFIER, token.type());
  EXPECT_EQ("--", token.identifier());

  EXPECT_FALSE(lexer.HasNextToken());
}

TEST(LexerTest, HyphenFollowedByComment) {
  MemFile mem_file("-#");
  Lexer lexer(mem_file.fp());

  Token token;
  lexer.GetNextToken(&token);
  EXPECT_EQ(Token::IDENTIFIER, token.type());
  EXPECT_EQ("-", token.identifier());

  EXPECT_FALSE(lexer.HasNextToken());
}

}  // namespace
}  // namespace toy_lang
}  // namespace floating_temple

int main(int argc, char** argv) {
  ParseCommandLineFlags(&argc, &argv, true);
  InitGoogleLogging(argv[0]);
  InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
