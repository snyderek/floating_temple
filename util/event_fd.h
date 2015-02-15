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

#ifndef UTIL_EVENT_FD_H_
#define UTIL_EVENT_FD_H_

namespace floating_temple {

void SignalEventFd(int event_fd);
void ClearEventFd(int event_fd);

}  // namespace floating_temple

#endif  // UTIL_EVENT_FD_H_
