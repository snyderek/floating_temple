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

#include "engine/peer_id.h"

#include <ostream>
#include <string>

#include <gflags/gflags.h>

#include "base/escape.h"
#include "base/logging.h"
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
using testing::Test;

namespace floating_temple {
namespace peer {
namespace {

TEST(PeerIdTest, MakePeerId) {
  EXPECT_EQ("ip/192.168.1.8/1025", MakePeerId("192.168.1.8", 1025));
}

class DoesntParseMatcher : public MatcherInterface<const string&> {
 public:
  bool MatchAndExplain(const string& s,
                       MatchResultListener* listener) const override {
    string address;
    int port = -1;
    return !ParsePeerId(s, &address, &port);
  }

  void DescribeTo(ostream* os) const override {
    *os << "doesn't parse as a peer id";
  }

  void DescribeNegationTo(ostream* os) const override {
    *os << "parses as a peer id";
  }
};

Matcher<const string&> DoesntParse() {
  return MakeMatcher(new DoesntParseMatcher());
}

class ParsesAsMatcher : public MatcherInterface<const string&> {
 public:
  ParsesAsMatcher(const string& expected_address, int expected_port)
      : expected_address_(expected_address),
        expected_port_(expected_port) {
  }

  bool MatchAndExplain(const string& s,
                       MatchResultListener* listener) const override {
    string address;
    int port = -1;
    if (!ParsePeerId(s, &address, &port)) {
      *listener << "parsing failed";
      return false;
    }

    if (address != expected_address_ || port != expected_port_) {
      *listener << "address == \"" << CEscape(address) << "\", port == "
                << port;
      return false;
    }

    return true;
  }

  void DescribeTo(ostream* os) const override {
    *os << "parses as a peer id (\"" << CEscape(expected_address_) << "\", "
        << expected_port_ << ")";
  }

  void DescribeNegationTo(ostream* os) const override {
    *os << "doesn't parse as a peer id (\"" << CEscape(expected_address_)
        << "\", " << expected_port_ << ")";
  }

 private:
  const string expected_address_;
  const int expected_port_;
};

Matcher<const string&> ParsesAs(const string& expected_address,
                                int expected_port) {
  return MakeMatcher(new ParsesAsMatcher(expected_address, expected_port));
}

TEST(PeerIdTest, ParsePeerId) {
  EXPECT_THAT("ip/192.168.1.8/0", ParsesAs("192.168.1.8", 0));
  EXPECT_THAT("ip/192.168.1.8/65535", ParsesAs("192.168.1.8", 65535));
  EXPECT_THAT("ip/ottawa/1025", ParsesAs("ottawa", 1025));
  EXPECT_THAT("ip/a/1025", ParsesAs("a", 1025));
  EXPECT_THAT("ip/192.168.1.8/00", ParsesAs("192.168.1.8", 0));
  EXPECT_THAT("ip/192.168.1.8/01025", ParsesAs("192.168.1.8", 1025));

  // Missing elements
  EXPECT_THAT("", DoesntParse());
  EXPECT_THAT("ip", DoesntParse());
  EXPECT_THAT("ip/", DoesntParse());
  EXPECT_THAT("ip//", DoesntParse());
  EXPECT_THAT("ip/192.168.1.8", DoesntParse());
  EXPECT_THAT("ip/192.168.1.8/", DoesntParse());
  EXPECT_THAT("ip//1025", DoesntParse());

  // Extra slash at the end
  EXPECT_THAT("ip/192.168.1.8/1025/", DoesntParse());

  // Port number out of range
  EXPECT_THAT("ip/192.168.1.8/-1", DoesntParse());
  EXPECT_THAT("ip/192.168.1.8/65536", DoesntParse());

  // Hexadecimal port number
  EXPECT_THAT("ip/192.168.1.8/8a", DoesntParse());
  EXPECT_THAT("ip/192.168.1.8/0x8a", DoesntParse());

  // Upper case prefix
  EXPECT_THAT("IP/192.168.1.8/1025", DoesntParse());
}

}  // namespace
}  // namespace peer
}  // namespace floating_temple

int main(int argc, char** argv) {
  ParseCommandLineFlags(&argc, &argv, true);
  InitGoogleLogging(argv[0]);
  InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
