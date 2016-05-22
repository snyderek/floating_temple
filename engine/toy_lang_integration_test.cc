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

#include <cstdio>
#include <string>
#include <vector>

#include <gflags/gflags.h>

#include "base/logging.h"
#include "include/c++/create_peer.h"
#include "include/c++/interpreter.h"
#include "include/c++/peer.h"
#include "third_party/gmock-1.7.0/gtest/include/gtest/gtest.h"
#include "toy_lang/interpreter_impl.h"
#include "toy_lang/run_toy_lang_program.h"
#include "util/string_util.h"
#include "util/tcp.h"

using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
using std::FILE;
using std::fclose;
using std::string;
using std::vector;
using testing::InitGoogleTest;
using testing::Test;

namespace floating_temple {
namespace engine {
namespace {

class ToyLangIntegrationTest : public Test {
 protected:
  void SetUp() override {
    interpreter_ = new toy_lang::InterpreterImpl();

    const vector<string> known_peer_ids;
    peer_ = CreateNetworkPeer(interpreter_, "toy_lang", GetLocalAddress(),
                              GetUnusedPortForTesting(), known_peer_ids, 1,
                              true);
  }

  void TearDown() override {
    peer_->Stop();
    delete interpreter_;
    delete peer_;
  }

  void RunTestProgram(const string& file_content) {
    char* const buffer = CreateCharBuffer(file_content);

    FILE* const fp = fmemopen(buffer, file_content.length(), "r");
    PLOG_IF(FATAL, fp == nullptr) << "fmemopen";

    toy_lang::RunToyLangFile(peer_, fp, false);

    PLOG_IF(FATAL, fclose(fp) != 0) << "fclose";
    delete[] buffer;
  }

  Peer* peer_;
  Interpreter* interpreter_;
};

TEST_F(ToyLangIntegrationTest, HelloWorld) {
  RunTestProgram("(print \"Hello, world.\")");
}

TEST_F(ToyLangIntegrationTest, BeginTran) {
  RunTestProgram("(begin_tran)");
}

TEST_F(ToyLangIntegrationTest, ExplicitTransaction) {
  const string kProgram =
      "(begin_tran)\n"
      "(end_tran)\n";

  RunTestProgram(kProgram);
}

// TODO(dss): Re-enable this test once the toy_lang interpreter is working
// again.
TEST_F(ToyLangIntegrationTest, DISABLED_FibList) {
  const string kProgram =
      "# Create a list that contains the Fibonacci sequence.\n"
      "\n"
      "(begin_tran)\n"
      "(if (map.is_set shared \"lst\") {\n"
      "  (set lst (map.get shared \"lst\"))\n"
      "} {\n"
      "  (set lst [0 1])\n"
      "  (map.set shared \"lst\" lst)\n"
      "})\n"
      "(end_tran)\n"
      "\n"
      "(while (lt (len lst) 20) {\n"
      "  (begin_tran)\n"
      "  (list.append lst (add (list.get lst -2) (list.get lst -1)))\n"
      "  (end_tran)\n"
      "\n"
      "  (begin_tran)\n"
      "  (print lst)\n"
      "  (end_tran)\n"
      "})\n";

  RunTestProgram(kProgram);
}

}  // namespace
}  // namespace engine
}  // namespace floating_temple

int main(int argc, char** argv) {
  ParseCommandLineFlags(&argc, &argv, true);
  InitGoogleLogging(argv[0]);
  InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
