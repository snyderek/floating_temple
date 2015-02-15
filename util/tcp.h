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

#ifndef UTIL_TCP_H_
#define UTIL_TCP_H_

#include <string>

namespace floating_temple {

std::string GetLocalAddress();
int ListenOnLocalAddress(const std::string& local_address, int port);
int ConnectToRemoteHost(const std::string& address, int port);

int GetUnusedPortForTesting();

}  // namespace floating_temple

#endif  // UTIL_TCP_H_
