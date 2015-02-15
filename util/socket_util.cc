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

#include "util/socket_util.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <cerrno>
#include <cstddef>
#include <cstring>
#include <string>

#include "base/logging.h"

using std::size_t;
using std::string;
using std::strncpy;

namespace floating_temple {
namespace {

void PopulateUnixAddrStruct(const string& socket_file_name, sockaddr_un* addr) {
  CHECK(addr != NULL);

  const size_t socket_file_name_length = socket_file_name.length();
  CHECK_GT(socket_file_name_length, 0u);
  CHECK_NE(socket_file_name[0], '\0');

  addr->sun_family = AF_UNIX;

  const size_t path_max = sizeof addr->sun_path;
  CHECK_LT(socket_file_name_length, path_max);
  strncpy(addr->sun_path, socket_file_name.c_str(), path_max);
  addr->sun_path[path_max - 1] = '\0';
}

}  // namespace

int ListenOnUnixSocket(const string& socket_file_name) {
  // Create the listen socket.
  const int listen_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
  CHECK_ERR(listen_fd) << " socket";

  // Set the listen socket to non-blocking mode.
  CHECK(SetFdToNonBlocking(listen_fd));

  // Bind the listen socket to the specified socket file name.
  sockaddr_un addr;
  PopulateUnixAddrStruct(socket_file_name, &addr);

  CHECK_ERR(bind(listen_fd, reinterpret_cast<const sockaddr*>(&addr),
                 static_cast<socklen_t>(sizeof addr)));

  // Put the socket in listen mode.
  // TODO(dss): Should the backlog value be set to something smaller?
  CHECK_ERR(listen(listen_fd, 128));

  return listen_fd;
}

int ConnectToUnixSocket(const string& socket_file_name) {
  // Create the connection socket.
  const int connection_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
  CHECK_ERR(connection_fd) << " socket";

  // Connect to the Unix-domain socket. Don't set the connection socket to
  // non-blocking mode yet, because that may cause connect() to fail with
  // EINPROGRESS.
  sockaddr_un addr;
  PopulateUnixAddrStruct(socket_file_name, &addr);

  CHECK_ERR(connect(connection_fd, reinterpret_cast<const sockaddr*>(&addr),
                    static_cast<socklen_t>(sizeof addr)));

  // Set the connection socket to non-blocking mode.
  CHECK(SetFdToNonBlocking(connection_fd));

  return connection_fd;
}

int AcceptConnection(int listen_fd, string* remote_address) {
  // Accept the connection.
  sockaddr_storage address;
  socklen_t address_length = sizeof address;

  const int connection_fd = accept4(listen_fd,
                                    reinterpret_cast<sockaddr*>(&address),
                                    &address_length, SOCK_CLOEXEC);
  if (connection_fd == -1) {
    PLOG_IF(FATAL, errno != EAGAIN && errno != EWOULDBLOCK) << "accept";
    return -1;
  }

  // Set the connection socket to non-blocking mode.
  CHECK(SetFdToNonBlocking(connection_fd));

  CHECK_LE(address_length, sizeof address);
  GetAddressString(reinterpret_cast<const sockaddr*>(&address), remote_address);

  return connection_fd;
}

void GetAddressString(const sockaddr* address, string* address_string) {
  CHECK(address_string != NULL);

  const int address_family = address->sa_family;

  const void* src = NULL;
  switch (address_family) {
    case AF_INET:
      src = &(reinterpret_cast<const sockaddr_in*>(address)->sin_addr);
      break;

    case AF_INET6:
      src = &(reinterpret_cast<const sockaddr_in6*>(address)->sin6_addr);
      break;

    case AF_UNIX:
      address_string->clear();
      return;

    default:
      LOG(FATAL) << "Unexpected address family: " << address_family;
  }

  char buffer[INET6_ADDRSTRLEN];
  PLOG_IF(FATAL, inet_ntop(address_family, src, buffer,
                           static_cast<socklen_t>(sizeof buffer)) == NULL)
      << "inet_ntop";

  address_string->assign(buffer);
}

bool SetFdToNonBlocking(int fd) {
  const int flags = fcntl(fd, F_GETFL);

  if (flags == -1) {
    PLOG(WARNING) << "fcntl(fd, F_GETFL)";
    return false;
  }

  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    PLOG(WARNING) << "fcntl(fd, F_SETFL, flags | O_NONBLOCK)";
    return false;
  }

  return true;
}

}  // namespace floating_temple
