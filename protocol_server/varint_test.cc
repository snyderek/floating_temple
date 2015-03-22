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

#include "protocol_server/varint.h"

#include <ostream>
#include <string>

#include <gflags/gflags.h>

#include "base/integral_types.h"
#include "base/logging.h"
#include "base/string_printf.h"
#include "third_party/gmock-1.7.0/gtest/include/gtest/gtest.h"
#include "third_party/gmock-1.7.0/include/gmock/gmock.h"

using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
using std::ostream;
using std::string;
using testing::InitGoogleTest;
using testing::MakeMatcher;
using testing::MatchResultListener;
using testing::Matcher;
using testing::MatcherInterface;

namespace floating_temple {
namespace {

string MakeString() {
  string s;
  return s;
}

string MakeString(char b0) {
  string s;
  s += b0;
  return s;
}

string MakeString(char b0, char b1) {
  string s;
  s += b0; s += b1;
  return s;
}

string MakeString(char b0, char b1, char b2) {
  string s;
  s += b0; s += b1; s += b2;
  return s;
}

string MakeString(char b0, char b1, char b2, char b3) {
  string s;
  s += b0; s += b1; s += b2; s += b3;
  return s;
}

string MakeString(char b0, char b1, char b2, char b3, char b4) {
  string s;
  s += b0; s += b1; s += b2; s += b3; s += b4;
  return s;
}

string MakeString(char b0, char b1, char b2, char b3, char b4, char b5) {
  string s;
  s += b0; s += b1; s += b2; s += b3; s += b4; s += b5;
  return s;
}

string MakeString(char b0, char b1, char b2, char b3, char b4, char b5,
                  char b6) {
  string s;
  s += b0; s += b1; s += b2; s += b3; s += b4; s += b5;
  s += b6;
  return s;
}

string MakeString(char b0, char b1, char b2, char b3, char b4, char b5,
                  char b6, char b7) {
  string s;
  s += b0; s += b1; s += b2; s += b3; s += b4; s += b5;
  s += b6; s += b7;
  return s;
}

string MakeString(char b0, char b1, char b2, char b3, char b4, char b5,
                  char b6, char b7, char b8) {
  string s;
  s += b0; s += b1; s += b2; s += b3; s += b4; s += b5;
  s += b6; s += b7; s += b8;
  return s;
}

string MakeString(char b0, char b1, char b2, char b3, char b4, char b5,
                  char b6, char b7, char b8, char b9) {
  string s;
  s += b0; s += b1; s += b2; s += b3; s += b4; s += b5;
  s += b6; s += b7; s += b8; s += b9;
  return s;
}

string FormatData(const string& data) {
  if (data.empty()) {
    return "()";
  }

  string s = "{ ";
  for (string::const_iterator it = data.begin(); it != data.end(); ++it) {
    if (it != data.begin()) {
      s += ", ";
    }
    StringAppendF(&s, "0x%02x",
                  static_cast<unsigned>(static_cast<unsigned char>(*it)));
  }
  s += " }";

  return s;
}

class DoesntParseMatcher : public MatcherInterface<const string&> {
 public:
  bool MatchAndExplain(const string& s,
                       MatchResultListener* listener) const override {
    uint64 value = 0u;
    return ParseVarint(s.data(), static_cast<int>(s.length()), &value) < 0;
  }

  void DescribeTo(ostream* os) const override {
    *os << "doesn't parse as a varint";
  }

  void DescribeNegationTo(ostream* os) const override {
    *os << "parses as a varint";
  }
};

Matcher<const string&> DoesntParse() {
  return MakeMatcher(new DoesntParseMatcher());
}

class ParsesAsMatcher : public MatcherInterface<const string&> {
 public:
  explicit ParsesAsMatcher(uint64 expected_value)
      : expected_value_(expected_value) {
  }

  bool MatchAndExplain(const string& s,
                       MatchResultListener* listener) const override {
    uint64 value = 0u;
    if (ParseVarint(s.data(), static_cast<int>(s.length()), &value) < 0) {
      *listener << "parsing failed";
      return false;
    }

    if (value != expected_value_) {
      *listener << "value == " << value;
      return false;
    }

    return true;
  }

  void DescribeTo(ostream* os) const override {
    *os << "parses as " << expected_value_;
  }

  void DescribeNegationTo(ostream* os) const override {
    *os << "doesn't parse as " << expected_value_;
  }

 private:
  const uint64 expected_value_;
};

Matcher<const string&> ParsesAs(uint64 expected_value) {
  return MakeMatcher(new ParsesAsMatcher(expected_value));
}

class FormatsAsMatcher : public MatcherInterface<uint64> {
 public:
  explicit FormatsAsMatcher(const string& expected_encoding)
      : expected_encoding_(expected_encoding) {
  }

  bool MatchAndExplain(uint64 value,
                       MatchResultListener* listener) const override {
    char buffer[kMaxVarintLength];
    const int buffer_size = static_cast<int>(sizeof buffer);

    const int length = FormatVarint(value, buffer, buffer_size);
    CHECK_LE(length, buffer_size);

    const string encoding(buffer, length);

    if (encoding != expected_encoding_) {
      *listener << "encoding == \"" << FormatData(encoding) << "\"";
      return false;
    }

    return true;
  }

  void DescribeTo(ostream* os) const override {
    *os << "formats as " << FormatData(expected_encoding_);
  }

  void DescribeNegationTo(ostream* os) const override {
    *os << "doesn't format as " << FormatData(expected_encoding_);
  }

 private:
  const string expected_encoding_;
};

Matcher<uint64> FormatsAs(const string& expected_encoding) {
  return MakeMatcher(new FormatsAsMatcher(expected_encoding));
}

TEST(VarintTest, ParseVarint) {
  EXPECT_THAT(MakeString(0x00), ParsesAs(0));
  EXPECT_THAT(MakeString(0x01), ParsesAs(1));
  EXPECT_THAT(MakeString(0x7f), ParsesAs(127));
  EXPECT_THAT(MakeString(0x80, 0x01), ParsesAs(128));
  EXPECT_THAT(MakeString(0x96, 0x01), ParsesAs(150));
  EXPECT_THAT(MakeString(0xac, 0x02), ParsesAs(300));
  EXPECT_THAT(MakeString(0xff, 0x7f), ParsesAs(16383));
  EXPECT_THAT(MakeString(0x80, 0x80, 0x01), ParsesAs(16384));
  EXPECT_THAT(MakeString(0xd2, 0x85, 0xd8, 0xcc, 0x04), ParsesAs(1234567890));
  EXPECT_THAT(MakeString(0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                         0x01),
              ParsesAs(18446744073709551615u));
  // Extra bytes after the varint
  EXPECT_THAT(MakeString(0xd2, 0x85, 0xd8, 0xcc, 0x04, 0x81, 0x01),
              ParsesAs(1234567890));

  // Zero-length string
  EXPECT_THAT(MakeString(), DoesntParse());
  // Incomplete varint
  EXPECT_THAT(MakeString(0xd2, 0x85, 0xd8, 0xcc), DoesntParse());
}

TEST(VarintTest, FormatVarint) {
  EXPECT_THAT(static_cast<uint64>(0), FormatsAs(MakeString(0x00)));
  EXPECT_THAT(static_cast<uint64>(1), FormatsAs(MakeString(0x01)));
  EXPECT_THAT(static_cast<uint64>(127), FormatsAs(MakeString(0x7f)));
  EXPECT_THAT(static_cast<uint64>(128), FormatsAs(MakeString(0x80, 0x01)));
  EXPECT_THAT(static_cast<uint64>(150), FormatsAs(MakeString(0x96, 0x01)));
  EXPECT_THAT(static_cast<uint64>(300), FormatsAs(MakeString(0xac, 0x02)));
  EXPECT_THAT(static_cast<uint64>(16383), FormatsAs(MakeString(0xff, 0x7f)));
  EXPECT_THAT(static_cast<uint64>(16384),
              FormatsAs(MakeString(0x80, 0x80, 0x01)));
  EXPECT_THAT(static_cast<uint64>(1234567890),
              FormatsAs(MakeString(0xd2, 0x85, 0xd8, 0xcc, 0x04)));
  EXPECT_THAT(static_cast<uint64>(18446744073709551615u),
              FormatsAs(MakeString(0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                   0xff, 0xff, 0x01)));
}

TEST(VarintTest, GetVarintLength) {
  EXPECT_EQ(1, GetVarintLength(0x0));
  EXPECT_EQ(1, GetVarintLength(0x1));
  EXPECT_EQ(1, GetVarintLength(0x7f));
  EXPECT_EQ(2, GetVarintLength(0x80));
  EXPECT_EQ(2, GetVarintLength(0x3fff));
  EXPECT_EQ(3, GetVarintLength(0x4000));
  EXPECT_EQ(3, GetVarintLength(0x1fffff));
  EXPECT_EQ(4, GetVarintLength(0x200000));
  EXPECT_EQ(4, GetVarintLength(0xfffffff));
  EXPECT_EQ(5, GetVarintLength(0x10000000));
  EXPECT_EQ(5, GetVarintLength(0x7ffffffff));
  EXPECT_EQ(6, GetVarintLength(0x800000000));
  EXPECT_EQ(6, GetVarintLength(0x2ffffffffff));
  EXPECT_EQ(7, GetVarintLength(0x40000000000));
  EXPECT_EQ(7, GetVarintLength(0x1ffffffffffff));
  EXPECT_EQ(8, GetVarintLength(0x2000000000000));
  EXPECT_EQ(8, GetVarintLength(0xffffffffffffff));
  EXPECT_EQ(9, GetVarintLength(0x100000000000000));
  EXPECT_EQ(9, GetVarintLength(0x7fffffffffffffff));
  EXPECT_EQ(10, GetVarintLength(0x8000000000000000));
  EXPECT_EQ(10, GetVarintLength(0xffffffffffffffff));
}

}  // namespace
}  // namespace floating_temple

int main(int argc, char** argv) {
  ParseCommandLineFlags(&argc, &argv, true);
  InitGoogleLogging(argv[0]);
  InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
