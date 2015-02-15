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

#include "peer/uuid_util.h"

#include <string>

#include <gflags/gflags.h>

#include "base/logging.h"
#include "peer/proto/uuid.pb.h"
#include "third_party/gmock-1.7.0/gtest/include/gtest/gtest.h"

using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
using std::string;
using testing::InitGoogleTest;

namespace floating_temple {
namespace peer {
namespace {

TEST(UuidTest, GeneratePredictableUuid) {
  Uuid ns_uuid;
  ns_uuid.set_high_word(0xd6501cb93914f24bu);
  ns_uuid.set_low_word(0xd4ef479bdf31e4f2u);

  Uuid uuid1, uuid2, uuid3;
  GeneratePredictableUuid(ns_uuid, "1", &uuid1);
  GeneratePredictableUuid(ns_uuid, "2", &uuid2);
  GeneratePredictableUuid(ns_uuid, "3", &uuid3);

  EXPECT_EQ("d664d9a2d3ae5bdeade2c613dc591fc2", UuidToString(uuid1));
  EXPECT_EQ("9fc8387748f75421b433ac98a643ed1e", UuidToString(uuid2));
  EXPECT_EQ("273061c7705c5900ae20ab56718ca765", UuidToString(uuid3));
}

TEST(UuidTest, UuidToString) {
  Uuid uuid;
  uuid.set_high_word(0x1424d6c1d6288486u);
  uuid.set_low_word(0x0b16c2844a6c6d64u);

  EXPECT_EQ("1424d6c1d62884860b16c2844a6c6d64", UuidToString(uuid));
}

TEST(UuidTest, StringToUuid) {
  const Uuid uuid = StringToUuid("18bb7fb4b43a4563739681c2aba2788d");

  EXPECT_EQ(0x18bb7fb4b43a4563u, uuid.high_word());
  EXPECT_EQ(0x739681c2aba2788du, uuid.low_word());
}

TEST(UuidTest, UuidToHyphenatedString) {
  Uuid uuid;
  uuid.set_high_word(0x8ca708e20f0f258eu);
  uuid.set_low_word(0x845eb6f614f0c6e7u);

  string s;
  UuidToHyphenatedString(uuid, &s);

  EXPECT_EQ("8ca708e2-0f0f-258e-845e-b6f614f0c6e7", s);
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
