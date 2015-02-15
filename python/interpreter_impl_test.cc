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

#include "python/interpreter_impl.h"

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "base/logging.h"
#include "third_party/gmock-1.7.0/gtest/include/gtest/gtest.h"
#include "util/tcp.h"

using google::InitGoogleLogging;
using std::FILE;
using std::fclose;
using std::memcpy;
using std::size_t;
using std::string;
using std::vector;
using testing::InitGoogleTest;
using testing::Test;

namespace floating_temple {
namespace python {
namespace {

class InterpreterImplTest : public Test {
 protected:
  virtual void SetUp() {
    const vector<string> known_peer_ids;

    interpreter_ = new InterpreterImpl();
    interpreter_->Start(GetUnusedPortForTesting(), known_peer_ids);
  }

  virtual void TearDown() {
    interpreter_->ShutDown();
    delete interpreter_;
  }

  bool RunProgram(const string& file_content, const string& file_name) {
    const size_t length = file_content.length();
    char* const buffer = new char[length];
    memcpy(buffer, file_content.data(), length);

    FILE* const fp = fmemopen(buffer, length, "r");
    PLOG_IF(FATAL, fp == NULL) << "fmemopen";

    const bool success = interpreter_->RunFile(fp, file_name);

    PLOG_IF(FATAL, fclose(fp) != 0) << "fclose";
    delete[] buffer;

    return success;
  }

  InterpreterImpl* interpreter_;
};

TEST_F(InterpreterImplTest, RunFibonacciProgram) {
  const string kProgramString =
      "# Fibonacci sequence\n"
      "\n"
      "a = 0\n"
      "b = 1\n"
      "\n"
      "for i in range(20):\n"
      "  print(a)\n"
      "  temp = a\n"
      "  a = b\n"
      "  b += temp\n"
      "\n";

  ASSERT_TRUE(RunProgram(kProgramString, "fibonacci-test"));
}

TEST_F(InterpreterImplTest, RunListProgram) {
  const string kProgramString =
      "lst = ['apple', 'banana']\n"
      "lst.append('cherry')\n"
      "print(' '.join(lst))\n";

  ASSERT_TRUE(RunProgram(kProgramString, "list-test"));
}

}  // namespace
}  // namespace python
}  // namespace floating_temple

int main(int argc, char** argv) {
  InitGoogleLogging(argv[0]);
  InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}