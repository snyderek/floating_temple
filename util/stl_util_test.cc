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

#include "util/stl_util.h"

#include <vector>

#include "base/logging.h"
#include "third_party/gmock-1.7.0/gtest/include/gtest/gtest.h"

using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
using std::vector;
using testing::InitGoogleTest;

namespace floating_temple {
namespace {

TEST(StlUtilTest, SimpleTest) {
  vector<int> vect(5);
  vect[0] = 1;
  vect[1] = 2;
  vect[2] = 3;
  vect[3] = 4;
  vect[4] = 5;

  // Const version
  EXPECT_EQ(3, *FindInContainer(static_cast<const vector<int>*>(&vect), 3));

  // Non-const version
  EXPECT_EQ(3, *FindInContainer(static_cast<vector<int>*>(&vect), 3));
}

}  // namespace
}  // namespace floating_temple

int main(int argc, char** argv) {
  ParseCommandLineFlags(&argc, &argv, true);
  InitGoogleLogging(argv[0]);
  InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
