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

#include "base/const_shared_ptr.h"

#include <string>

#include <gflags/gflags.h>

#include "base/logging.h"
#include "base/macros.h"
#include "third_party/gmock-1.7.0/gtest/include/gtest/gtest.h"

using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
using std::string;
using testing::InitGoogleTest;

namespace floating_temple {
namespace {

class TestObject {
 public:
  TestObject(const string& str, int number)
      : str_(str),
        number_(number) {
  }

  const string& str() const { return str_; }
  int number() const { return number_; }

 private:
  const string str_;
  const int number_;

  DISALLOW_COPY_AND_ASSIGN(TestObject);
};

TEST(SharedPtrTest, Destructor) {
  const_shared_ptr<TestObject> a(new TestObject("oxygen", 8));
  {
    const_shared_ptr<TestObject> b(a);
    EXPECT_EQ("oxygen", b->str());
  }
  EXPECT_EQ("oxygen", a->str());
}

TEST(SharedPtrTest, ResetFromNullToNull) {
  const_shared_ptr<TestObject> a(nullptr);
  const_shared_ptr<TestObject> b(a);
  a.reset(nullptr);
  EXPECT_TRUE(a.get() == nullptr);
  EXPECT_TRUE(b.get() == nullptr);
}

TEST(SharedPtrTest, ResetFromNullToNonNull) {
  const_shared_ptr<TestObject> a(nullptr);
  const_shared_ptr<TestObject> b(a);
  a.reset(new TestObject("cobalt", 27));
  EXPECT_EQ("cobalt", a->str());
  EXPECT_TRUE(b.get() == nullptr);
}

TEST(SharedPtrTest, ResetFromNonNullToNull) {
  const_shared_ptr<TestObject> a(new TestObject("niobium", 41));
  const_shared_ptr<TestObject> b(a);
  a.reset(nullptr);
  EXPECT_TRUE(a.get() == nullptr);
  EXPECT_EQ("niobium", b->str());
}

TEST(SharedPtrTest, ResetFromNonNullToNonNull) {
  const_shared_ptr<TestObject> a(new TestObject("astatine", 85));
  const_shared_ptr<TestObject> b(a);
  a.reset(new TestObject("xenon", 54));
  EXPECT_EQ("xenon", a->str());
  EXPECT_EQ("astatine", b->str());
}

TEST(SharedPtrTest, AssignmentFromNullToNull) {
  const_shared_ptr<TestObject> a(nullptr);
  const_shared_ptr<TestObject> b(a);
  const_shared_ptr<TestObject> c(nullptr);
  b = c;
  EXPECT_TRUE(b.get() == nullptr);
}

TEST(SharedPtrTest, AssignmentFromNullToNonNull) {
  const_shared_ptr<TestObject> a(nullptr);
  const_shared_ptr<TestObject> b(a);
  const_shared_ptr<TestObject> c(new TestObject("thallium", 81));
  b = c;
  EXPECT_EQ("thallium", b->str());
}

TEST(SharedPtrTest, AssignmentFromNonNullToNull) {
  const_shared_ptr<TestObject> a(new TestObject("lithium", 3));
  const_shared_ptr<TestObject> b(a);
  const_shared_ptr<TestObject> c(nullptr);
  b = c;
  EXPECT_TRUE(b.get() == nullptr);
}

TEST(SharedPtrTest, AssignmentFromNonNullToNonNull) {
  const_shared_ptr<TestObject> a(new TestObject("scandium", 21));
  const_shared_ptr<TestObject> b(a);
  const_shared_ptr<TestObject> c(new TestObject("rhodium", 45));
  b = c;
  EXPECT_EQ("rhodium", b->str());
}

}  // namespace
}  // namespace floating_temple

int main(int argc, char** argv) {
  ParseCommandLineFlags(&argc, &argv, true);
  InitGoogleLogging(argv[0]);
  InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
