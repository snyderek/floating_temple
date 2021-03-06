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

#ifndef ENGINE_PEER_ID_H_
#define ENGINE_PEER_ID_H_

#include <string>

namespace floating_temple {
namespace engine {

std::string MakePeerId(const std::string& address, int port);
bool ParsePeerId(const std::string& peer_id, std::string* address, int* port);

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_PEER_ID_H_
