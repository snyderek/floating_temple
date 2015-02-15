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

#include "protocol_server/protocol_server.h"

#include <gflags/gflags.h>

DEFINE_int32(protocol_connection_timeout_sec_for_debugging, -1,
             "If this flag is set, the process will crash if it needs to wait "
             "more than the specified number of seconds to send or receive "
             "data on a protocol connection. (For debugging only.)");
