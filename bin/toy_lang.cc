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

#include <string>

#include <gflags/gflags.h>

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "include/c++/create_peer.h"
#include "include/c++/peer.h"
#include "toy_lang/run_toy_lang_program.h"

using floating_temple::CreateStandalonePeer;
using floating_temple::Peer;
using floating_temple::scoped_ptr;
using floating_temple::toy_lang::RunToyLangProgram;
using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
using google::SetUsageMessage;
using google::SetVersionString;

int main(int argc, char* argv[]) {
  SetUsageMessage("Stand-alone interpreter for the toy language.\n"
                  "\n"
                  "Sample usage:\n"
                  "\n"
                  "toy_lang sample.toy");
  SetVersionString("0.1");
  ParseCommandLineFlags(&argc, &argv, true);
  InitGoogleLogging(argv[0]);

  CHECK_EQ(argc, 2)
      << "You must specify exactly one source file on the command line.";

  const scoped_ptr<Peer> peer(CreateStandalonePeer());
  RunToyLangProgram(peer.get(), argv[1]);
  peer->Stop();

  return 0;
}
