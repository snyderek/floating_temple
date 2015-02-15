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

#include "peer/interval_map.h"

#include <vector>

#include <gflags/gflags.h>

#include "base/logging.h"
#include "third_party/gmock-1.7.0/gtest/include/gtest/gtest.h"
#include "third_party/gmock-1.7.0/include/gmock/gmock.h"

using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
using std::vector;
using testing::ElementsAre;
using testing::InitGoogleTest;
using testing::Test;

namespace floating_temple {
namespace peer {
namespace {

class IntervalMapTest : public Test {
 protected:
  vector<int> GetEndPoints() const {
    vector<int> end_points;
    interval_map_.GetEndPoints(&end_points);
    return end_points;
  }

  IntervalMap<int> interval_map_;
};

TEST_F(IntervalMapTest, AddToEmptyMap) {
  interval_map_.AddInterval(2, 5);

  EXPECT_THAT(GetEndPoints(), ElementsAre(2, 5));
}

TEST_F(IntervalMapTest, JoinIntervals) {
  interval_map_.AddInterval(2, 5);
  interval_map_.AddInterval(8, 10);
  interval_map_.AddInterval(5, 8);

  EXPECT_THAT(GetEndPoints(), ElementsAre(2, 10));
}

TEST_F(IntervalMapTest, JoinIntervalsWithOverlap) {
  interval_map_.AddInterval(2, 5);
  interval_map_.AddInterval(8, 10);
  interval_map_.AddInterval(4, 9);

  EXPECT_THAT(GetEndPoints(), ElementsAre(2, 10));
}

TEST_F(IntervalMapTest, DistinctIntervals) {
  interval_map_.AddInterval(5, 8);
  interval_map_.AddInterval(9, 10);
  interval_map_.AddInterval(1, 4);

  EXPECT_THAT(GetEndPoints(), ElementsAre(1, 4, 5, 8, 9, 10));
}

TEST_F(IntervalMapTest, EmptyMapContains) {
  EXPECT_FALSE(interval_map_.Contains(0));
  EXPECT_FALSE(interval_map_.Contains(5));
}

TEST_F(IntervalMapTest, SingleIntervalContains) {
  interval_map_.AddInterval(2, 5);

  EXPECT_FALSE(interval_map_.Contains(1));
  EXPECT_TRUE(interval_map_.Contains(2));
  EXPECT_TRUE(interval_map_.Contains(3));
  EXPECT_TRUE(interval_map_.Contains(4));
  EXPECT_FALSE(interval_map_.Contains(5));
  EXPECT_FALSE(interval_map_.Contains(6));
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
