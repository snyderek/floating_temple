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

#include "protocol_server/buffer_util.h"

#include <string>

#include <gflags/gflags.h>

#include "base/logging.h"
#include "third_party/gmock-1.7.0/gtest/include/gtest/gtest.h"

using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
using std::string;
using testing::InitGoogleTest;
using testing::Test;

namespace floating_temple {
namespace {

class StringTest : public Test {
 public:
  explicit StringTest(const string& s)
    : s_(s),
      start_(s_.data()),
      end_(s_.data() + s.length()),
      length_(static_cast<int>(s.length())) {
  }

 protected:
  const string s_;
  const char* const start_;
  const char* const end_;
  const int length_;
};

class BasicStringTest : public StringTest {
 public:
  BasicStringTest() : StringTest("TEST STRING") {
  }
};

TEST_F(BasicStringTest, NormalCase) {
  // Single occurrence
  EXPECT_EQ(start_ + 1, FindCharInRange(start_, end_, 'E'));

  // Multiple occurrences
  EXPECT_EQ(start_ + 2, FindCharInRange(start_, end_, 'S'));

  // First and last characters
  EXPECT_EQ(start_, FindCharInRange(start_, end_, 'T'));
  EXPECT_EQ(start_ + 10, FindCharInRange(start_, end_, 'G'));

  // Not found
  EXPECT_EQ(end_, FindCharInRange(start_, end_, 'x'));
}

TEST_F(BasicStringTest, EmptyString) {
  EXPECT_EQ(start_, FindCharInRange(start_, start_, 'S'));
}

TEST_F(BasicStringTest, NullCharNotFound) {
  EXPECT_EQ(end_, FindCharInRange(start_, end_, '\0'));
}

class NullCharStringTest : public StringTest {
 public:
  NullCharStringTest() : StringTest(string("TEST\0STRING", 11)) {
  }
};

TEST_F(NullCharStringTest, NullCharFound) {
  EXPECT_EQ(start_ + 4, FindCharInRange(start_, end_, '\0'));
}

TEST_F(NullCharStringTest, SearchPastNullChar) {
  EXPECT_EQ(start_ + 7, FindCharInRange(start_, end_, 'R'));
}

}  // namespace
}  // namespace floating_temple

int main(int argc, char** argv) {
  ParseCommandLineFlags(&argc, &argv, true);
  InitGoogleLogging(argv[0]);
  InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
