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

#include "peer/peer_id.h"

#include <cctype>
#include <string>

#include "base/logging.h"
#include "base/string_printf.h"

using std::isdigit;
using std::string;

namespace floating_temple {
namespace peer {
namespace {

enum ParseState { PREFIX1, PREFIX2, PREFIX3, ADDRESS_FIRST, ADDRESS_REST,
                  PORT_FIRST, PORT_REST };

}  // namespace

string MakePeerId(const string& address, int port) {
  return StringPrintf("ip/%s/%d", address.c_str(), port);
}

bool ParsePeerId(const string& peer_id, string* address, int* port) {
  CHECK(address != nullptr);
  CHECK(port != nullptr);

  string address_temp;
  int port_temp = 0;

  ParseState state = PREFIX1;

  for (const char c : peer_id) {
    switch (state) {
      case PREFIX1:
        if (c != 'i') {
          return false;
        }
        state = PREFIX2;
        break;

      case PREFIX2:
        if (c != 'p') {
          return false;
        }
        state = PREFIX3;
        break;

      case PREFIX3:
        if (c != '/') {
          return false;
        }
        state = ADDRESS_FIRST;
        break;

      case ADDRESS_FIRST:
        if (c == '/') {
          return false;
        }
        address_temp += c;
        state = ADDRESS_REST;
        break;

      case ADDRESS_REST:
        if (c == '/') {
          state = PORT_FIRST;
        } else {
          address_temp += c;
        }
        break;

      case PORT_FIRST:
        if (!isdigit(c)) {
          return false;
        }
        port_temp = static_cast<int>(c - '0');
        state = PORT_REST;
        break;

      case PORT_REST:
        if (!isdigit(c)) {
          return false;
        }
        port_temp = port_temp * 10 + static_cast<int>(c - '0');
        if (port_temp > 65535) {
          return false;
        }
        break;

      default:
        LOG(FATAL) << "Invalid state value: " << static_cast<int>(state);
    }
  }

  if (state != PORT_REST) {
    return false;
  }

  address->swap(address_temp);
  *port = port_temp;

  return true;
}

}  // namespace peer
}  // namespace floating_temple
