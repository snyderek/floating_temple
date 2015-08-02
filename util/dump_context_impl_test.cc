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

#include "util/dump_context_impl.h"

#include <string>

#include "base/logging.h"
#include "third_party/gmock-1.7.0/gtest/include/gtest/gtest.h"

using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
using std::string;
using testing::InitGoogleTest;

namespace floating_temple {
namespace {

TEST(DumpContextImplTest, AlmostEverything) {
  DumpContextImpl dc;

  dc.BeginMap();
    dc.AddString("hello");
    dc.AddBool(false);

    dc.AddString("good\tbye");
    dc.BeginList();
      dc.AddInt(-123);
      dc.AddInt(0);
      dc.AddInt(456);
    dc.End();
  dc.End();

  string json;
  dc.FormatJson(&json);

  EXPECT_EQ("{ \"hello\": false, \"good\\tbye\": [ -123, 0, 456 ] }", json);
}

TEST(DumpContextImplTest, SingleValue) {
  DumpContextImpl dc;
  dc.AddString("abc");

  string json;
  dc.FormatJson(&json);

  EXPECT_EQ("\"abc\"", json);
}

}  // namespace
}  // namespace floating_temple

int main(int argc, char** argv) {
  ParseCommandLineFlags(&argc, &argv, true);
  InitGoogleLogging(argv[0]);
  InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
