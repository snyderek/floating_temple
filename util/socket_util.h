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

#ifndef UTIL_SOCKET_UTIL_H_
#define UTIL_SOCKET_UTIL_H_

#include <sys/socket.h>

#include <string>

namespace floating_temple {

int ListenOnUnixSocket(const std::string& socket_file_name);
int ConnectToUnixSocket(const std::string& socket_file_name);

int AcceptConnection(int listen_fd, std::string* remote_address);
void GetAddressString(const sockaddr* address, std::string* address_string);

bool SetFdToNonBlocking(int fd);

}  // namespace floating_temple

#endif  // UTIL_SOCKET_UTIL_H_
