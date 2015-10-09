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

// TODO(dss): This source file duplicates much of the code from
// bin/floating_toy_lang.cc. Consider factoring out the common code into a class
// or function.

#include <memory>
#include <string>
#include <vector>

#include <gflags/gflags.h>

#include "base/logging.h"
#include "include/c++/create_peer.h"
#include "include/c++/peer.h"
#include "lua/interpreter_impl.h"
#include "lua/run_lua_program.h"
#include "util/comma_separated.h"
#include "util/signal_handler.h"
#include "util/tcp.h"

using floating_temple::CreateNetworkPeer;
using floating_temple::GetLocalAddress;
using floating_temple::InstallSignalHandler;
using floating_temple::ParseCommaSeparatedList;
using floating_temple::Peer;
using floating_temple::lua::InterpreterImpl;
using floating_temple::lua::RunLuaProgram;
using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
using google::SetUsageMessage;
using google::SetVersionString;
using std::string;
using std::unique_ptr;
using std::vector;

DEFINE_int32(peer_port, -1, "Port number for the peer's TCP server");
DEFINE_string(known_peers, "",
              "Comma-separated list of peer IDs of other known peers");
DEFINE_int32(send_receive_thread_count, 1,
             "The number of threads to use for processing socket connections.");
DEFINE_bool(linger, true,
            "Don't exit the process until SIGTERM is received. If this flag is "
            "set to false, the process will exit immediately after the Lua "
            "program has finished executing.");

int main(int argc, char* argv[]) {
  SetUsageMessage("Distributed interpreter for the Lua language.\n"
                  "\n"
                  "Sample usage:\n"
                  "\n"
                  "floating_lua --peer_port=1025 sample.lua");
  SetVersionString("0.1");
  ParseCommandLineFlags(&argc, &argv, true);
  InitGoogleLogging(argv[0]);

  // Parse and validate command line flags.
  CHECK_GE(FLAGS_peer_port, 0)
      << "You must specify a peer port number using the --peer_port flag.";
  CHECK_LT(FLAGS_peer_port, 65536);

  vector<string> known_peer_ids;
  ParseCommaSeparatedList(FLAGS_known_peers, &known_peer_ids);

  // Validate command line parameters.
  // TODO(dss): Allow the interpreter to be used interactively.
  CHECK_EQ(argc, 2)
      << "You must specify exactly one source file on the command line.";

  // Install signal handlers for SIGINT and SIGTERM.
  InstallSignalHandler();

  // Start the local interpreter.
  InterpreterImpl interpreter;
  interpreter.Init();

  // Start the peer.
  LOG(WARNING) << "Starting peer...";
  const unique_ptr<Peer> peer(
      CreateNetworkPeer(&interpreter, "lua52", GetLocalAddress(),
                        FLAGS_peer_port, known_peer_ids,
                        FLAGS_send_receive_thread_count, false));
  LOG(WARNING) << "Peer started.";

  // Run the source file.
  const int return_code = RunLuaProgram(peer.get(), argv[1], FLAGS_linger);
  LOG(WARNING) << "The program has completed successfully";

  // Stop the peer.
  LOG(WARNING) << "Stopping peer...";
  peer->Stop();
  LOG(WARNING) << "Peer stopped.";

  return return_code;
}
