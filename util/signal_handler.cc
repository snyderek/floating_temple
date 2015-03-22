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

#include "util/signal_handler.h"

#include <cerrno>
#include <csignal>

#include "base/logging.h"

namespace floating_temple {
namespace {

// This flag is set to true when Ctrl-C or SIGTERM is received.
volatile bool g_exit_requested = false;

void HandleSignal(int signal_number) {
  g_exit_requested = true;
}

}  // namespace

void InstallSignalHandler() {
  // Block SIGINT and SIGTERM until this process is ready to handle them.
  sigset_t signal_mask;
  CHECK_ERR(sigemptyset(&signal_mask));
  CHECK_ERR(sigaddset(&signal_mask, SIGINT));
  CHECK_ERR(sigaddset(&signal_mask, SIGTERM));
  CHECK_ERR(sigprocmask(SIG_BLOCK, &signal_mask, nullptr));

  // Install signal handlers for SIGINT and SIGTERM.
  struct sigaction signal_action;
  signal_action.sa_handler = &HandleSignal;
  signal_action.sa_mask = signal_mask;
  signal_action.sa_flags = 0;

  CHECK_ERR(sigaction(SIGINT, &signal_action, nullptr));
  CHECK_ERR(sigaction(SIGTERM, &signal_action, nullptr));
}

void WaitForSignal() {
  while (!g_exit_requested) {
    // Temporarily re-enable all signals and wait for a signal to be received.
    sigset_t signal_mask;
    CHECK_ERR(sigemptyset(&signal_mask));
    CHECK_EQ(sigsuspend(&signal_mask), -1);
    CHECK_EQ(errno, EINTR);
  }
}

}  // namespace floating_temple
