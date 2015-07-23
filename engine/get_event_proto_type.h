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

#ifndef ENGINE_GET_EVENT_PROTO_TYPE_H_
#define ENGINE_GET_EVENT_PROTO_TYPE_H_

#include "engine/proto/event.pb.h"

namespace floating_temple {
namespace engine {

EventProto::Type GetEventProtoType(const EventProto& event_proto);

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_GET_EVENT_PROTO_TYPE_H_
