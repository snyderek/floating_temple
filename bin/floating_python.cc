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

// TODO(dss): This source file duplicates some of the code from
// bin/floating_toy_lang.cc. Consider factoring out the common code into a class
// or function.

#include <string>
#include <vector>

#include <gflags/gflags.h>

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "include/c++/create_peer.h"
#include "include/c++/peer.h"
#include "python/interpreter_impl.h"
#include "python/run_python_program.h"
#include "util/comma_separated.h"
#include "util/signal_handler.h"
#include "util/tcp.h"

using floating_temple::CreateNetworkPeer;
using floating_temple::GetLocalAddress;
using floating_temple::INFO;
using floating_temple::InstallSignalHandler;
using floating_temple::ParseCommaSeparatedList;
using floating_temple::Peer;
using floating_temple::WaitForSignal;
using floating_temple::python::InterpreterImpl;
using floating_temple::python::RunPythonProgram;
using floating_temple::scoped_ptr;
using google::FlushLogFiles;
using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
using google::SetUsageMessage;
using google::SetVersionString;
using std::string;
using std::vector;

DEFINE_int32(peer_port, -1, "Port number for the peer's TCP server");
DEFINE_string(known_peers, "",
              "Comma-separated list of peer IDs of other known peers");
DEFINE_int32(send_receive_thread_count, 1,
             "The number of threads to use for processing socket connections.");
DEFINE_bool(linger, true,
            "Don't exit the process until SIGTERM is received. If this flag is "
            "set to false, the process will exit immediately after the Python "
            "program has finished executing.");

int main(int argc, char* argv[]) {
  SetUsageMessage("Distributed interpreter for the Python language.\n"
                  "\n"
                  "Sample usage:\n"
                  "\n"
                  "floating_python --peer_port=1025 sample.py");
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

  // Start the peer.
  LOG(INFO) << "Starting peer...";
  const scoped_ptr<Peer> peer(
      CreateNetworkPeer(&interpreter, "python3", GetLocalAddress(),
                        FLAGS_peer_port, known_peer_ids,
                        FLAGS_send_receive_thread_count, true));
  LOG(INFO) << "Peer started.";

  FlushLogFiles(INFO);

  // Run the source file.
  RunPythonProgram(peer.get(), argv[1]);

  if (FLAGS_linger) {
    // Wait until this process receives a request to exit.
    WaitForSignal();
  }

  // Stop the peer.
  LOG(INFO) << "Stopping peer...";
  peer->Stop();
  LOG(INFO) << "Peer stopped.";

  return 0;
}
