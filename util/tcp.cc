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

#include "util/tcp.h"

#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <string>

#include "base/logging.h"
#include "base/random.h"
#include "base/string_printf.h"
#include "util/socket_util.h"

using std::memset;
using std::string;

namespace floating_temple {
namespace {

addrinfo* GetAddressInfo(const string& address, int port) {
  CHECK(!address.empty());

  addrinfo hints;
  memset(&hints, 0, sizeof hints);
  hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;
  hints.ai_addrlen = 0;
  hints.ai_addr = nullptr;
  hints.ai_canonname = nullptr;
  hints.ai_next = nullptr;

  string port_string;
  StringAppendF(&port_string, "%d", port);

  addrinfo* result = nullptr;
  const int error_code = getaddrinfo(address.c_str(), port_string.c_str(),
                                     &hints, &result);
  CHECK_EQ(error_code, 0) << "getaddrinfo: " << gai_strerror(error_code);
  CHECK(result != nullptr);

  return result;
}

int BindToAddress(const addrinfo* ai) {
  const int fd = socket(ai->ai_family, ai->ai_socktype | SOCK_CLOEXEC,
                        ai->ai_protocol);
  if (fd == -1) {
    PLOG(WARNING) << "socket";
    return -1;
  }

  if (!SetFdToNonBlocking(fd)) {
    CHECK_ERR(close(fd));
    return -1;
  }

  if (bind(fd, ai->ai_addr, static_cast<socklen_t>(ai->ai_addrlen)) == -1) {
    PLOG(WARNING) << "bind";
    CHECK_ERR(close(fd));
    return -1;
  }

  return fd;
}

int ConnectToAddress(const addrinfo* ai) {
  const int fd = socket(ai->ai_family, ai->ai_socktype | SOCK_CLOEXEC,
                        ai->ai_protocol);
  if (fd == -1) {
    PLOG(WARNING) << "socket";
    return -1;
  }

  if (connect(fd, ai->ai_addr, ai->ai_addrlen) == -1) {
    PLOG(WARNING) << "connect";
    CHECK_ERR(close(fd));
    return -1;
  }

  // Set the connected socket to non-blocking mode.
  if (!SetFdToNonBlocking(fd)) {
    CHECK_ERR(close(fd));
    return -1;
  }

  return fd;
}

int BindToSomeAddress(const addrinfo* ai_list) {
  CHECK(ai_list != nullptr);

  for (const addrinfo* ai = ai_list; ai != nullptr; ai = ai->ai_next) {
    const int socket_fd = BindToAddress(ai);

    if (socket_fd != -1) {
      return socket_fd;
    }
  }

  return -1;
}

int ConnectToSomeAddress(const addrinfo* ai_list) {
  CHECK(ai_list != nullptr);

  for (const addrinfo* ai = ai_list; ai != nullptr; ai = ai->ai_next) {
    const int socket_fd = ConnectToAddress(ai);

    if (socket_fd != -1) {
      return socket_fd;
    }
  }

  return -1;
}

}  // namespace

string GetLocalAddress() {
  ifaddrs* ifa_list = nullptr;
  CHECK_ERR(getifaddrs(&ifa_list));
  CHECK(ifa_list != nullptr);

  int selected_family = AF_UNSPEC;
  string address_string;

  for (const ifaddrs* ifa = ifa_list; ifa != nullptr; ifa = ifa->ifa_next) {
    if ((ifa->ifa_flags & IFF_LOOPBACK) == 0) {
      const sockaddr* const address = ifa->ifa_addr;
      const int family = address->sa_family;

      if ((family == AF_INET && selected_family != AF_INET) ||
          (family == AF_INET6 && selected_family != AF_INET &&
           selected_family != AF_INET6)) {
        selected_family = family;
        GetAddressString(address, &address_string);
      }
    }
  }

  freeifaddrs(ifa_list);

  CHECK_NE(selected_family, AF_UNSPEC);
  CHECK(!address_string.empty());

  return address_string;
}

int ListenOnLocalAddress(const string& local_address, int port) {
  addrinfo* const address_info = GetAddressInfo(local_address, port);
  const int socket_fd = BindToSomeAddress(address_info);
  freeaddrinfo(address_info);

  CHECK_NE(socket_fd, -1) << "Could not bind to any local address on port "
                          << port;

  // TODO(dss): Should the backlog value be set to something smaller?
  CHECK_ERR(listen(socket_fd, 128));

  return socket_fd;
}

int ConnectToRemoteHost(const string& address, int port) {
  addrinfo* const address_info = GetAddressInfo(address, port);
  const int socket_fd = ConnectToSomeAddress(address_info);
  freeaddrinfo(address_info);

  LOG_IF(WARNING, socket_fd == -1)
      << "Could not connect to " << address << " port " << port;

  return socket_fd;
}

int GetUnusedPortForTesting() {
  static const int kMinPort = 1024;
  static const int kMaxPort = 65535;

  static int port = -1;

  const string local_address = GetLocalAddress();

  if (port == -1) {
    port = GetRandomInt() % (kMaxPort - kMinPort + 1) + kMinPort;
  }

  for (;;) {
    ++port;
    if (port > kMaxPort) {
      port = kMinPort;
    }

    addrinfo* const address_info = GetAddressInfo(local_address, port);
    const int socket_fd = BindToSomeAddress(address_info);
    freeaddrinfo(address_info);

    if (socket_fd != -1) {
      CHECK_ERR(close(socket_fd));
      LOG(INFO) << "Found unused port: " << port;
      return port;
    }
  }

  return -1;
}

}  // namespace floating_temple
