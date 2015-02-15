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

#include "base/string_printf.h"

#include <string>

#include "base/logging.h"
#include "third_party/gmock-1.7.0/gtest/include/gtest/gtest.h"

using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
using std::string;
using testing::InitGoogleTest;

namespace floating_temple {
namespace {

string MakeString(string::size_type requested_length) {
  // This string is 100 characters long.
  const string kTestString =
      "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrst"
      "uvwxyzabcdefghijklmnopqrstuv";
  CHECK_EQ(kTestString.length(), 100u);

  string s;
  while (s.length() < requested_length) {
    s += kTestString.substr(0u, requested_length - s.length());
  }

  CHECK_EQ(s.length(), requested_length);

  return s;
}

TEST(StringPrintfTest, EmptyString) {
  EXPECT_EQ("", StringPrintf("%s", ""));
}

TEST(StringPrintfTest, Length999) {
  const string formatted = StringPrintf("%s", MakeString(999u).c_str());
  EXPECT_EQ(999u, formatted.length());
  EXPECT_EQ('u', formatted[998u]);
}

TEST(StringPrintfTest, Length1000) {
  const string formatted = StringPrintf("%s", MakeString(1000u).c_str());
  EXPECT_EQ(1000u, formatted.length());
  EXPECT_EQ('v', formatted[999u]);
}

TEST(StringPrintfTest, Length1001) {
  const string formatted = StringPrintf("%s", MakeString(1001u).c_str());
  EXPECT_EQ(1001u, formatted.length());
  EXPECT_EQ('a', formatted[1000u]);
}

TEST(StringPrintfTest, Length100000) {
  const string formatted = StringPrintf("%s", MakeString(100000u).c_str());
  EXPECT_EQ(100000u, formatted.length());
  EXPECT_EQ('v', formatted[99999u]);
}

}  // namespace
}  // namespace floating_temple

int main(int argc, char** argv) {
  ParseCommandLineFlags(&argc, &argv, true);
  InitGoogleLogging(argv[0]);
  InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
