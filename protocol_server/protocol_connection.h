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

#ifndef PROTOCOL_SERVER_PROTOCOL_CONNECTION_H_
#define PROTOCOL_SERVER_PROTOCOL_CONNECTION_H_

namespace floating_temple {

class ProtocolConnection {
 public:
  virtual ~ProtocolConnection() {}

  virtual void Close() = 0;

  virtual void NotifyMessageReadyToSend() = 0;
};

}  // namespace floating_temple

#endif  // PROTOCOL_SERVER_PROTOCOL_CONNECTION_H_