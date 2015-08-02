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

// Convenience functions for using event FDs. Event FDs are useful in Linux
// because the select() function can only wait on file descriptors. To wait on
// some other event (e.g., a shutdown notification), create an event FD and pass
// it to select(). Another thread can then signal the event FD to wake the
// select thread. For more information, see the man page for eventfd(2).

// Signals the event FD by writing an increment of 1 to it. Crashes if an error
// occurs.
void SignalEventFd(int event_fd);

// Resets the event FD by reading its value. Crashes if an error other than
// EAGAIN or EWOULDBLOCK occurs.
void ClearEventFd(int event_fd);

}  // namespace floating_temple

#endif  // UTIL_EVENT_FD_H_
