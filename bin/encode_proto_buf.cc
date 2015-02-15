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

#include "base/escape.h"
#include "base/logging.h"
#include "peer/proto/peer.pb.h"

using floating_temple::CEscape;
using floating_temple::peer::PeerMessage;
using floating_temple::peer::TestMessage;
using google::InitGoogleLogging;
using std::printf;
using std::string;

int main(int argc, char** argv) {
  InitGoogleLogging(argv[0]);

  PeerMessage peer_message;
  TestMessage* const test_message = peer_message.mutable_test_message();
  test_message->set_text(
      "In my younger and more vulnerable years my father gave me some advice "
      "that I've been turning over in my mind ever since.");

  string encoded_string;
  CHECK(peer_message.SerializeToString(&encoded_string));

  printf("\"%s\"\n", CEscape(encoded_string).c_str());

  return 0;
}
