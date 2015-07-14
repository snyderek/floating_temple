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

#include "engine/get_event_proto_type.h"

#include "base/logging.h"
#include "engine/proto/event.pb.h"

namespace floating_temple {
namespace peer {

#define CHECK_FIELD(has_method, enum_const) \
  if (event_proto.has_method()) { \
    CHECK_EQ(type, EventProto::UNKNOWN); \
    type = EventProto::enum_const; \
  }

EventProto::Type GetEventProtoType(const EventProto& event_proto) {
  EventProto::Type type = EventProto::UNKNOWN;

  CHECK_FIELD(has_object_creation, OBJECT_CREATION);
  CHECK_FIELD(has_sub_object_creation, SUB_OBJECT_CREATION);
  CHECK_FIELD(has_begin_transaction, BEGIN_TRANSACTION);
  CHECK_FIELD(has_end_transaction, END_TRANSACTION);
  CHECK_FIELD(has_method_call, METHOD_CALL);
  CHECK_FIELD(has_method_return, METHOD_RETURN);
  CHECK_FIELD(has_sub_method_call, SUB_METHOD_CALL);
  CHECK_FIELD(has_sub_method_return, SUB_METHOD_RETURN);
  CHECK_FIELD(has_self_method_call, SELF_METHOD_CALL);
  CHECK_FIELD(has_self_method_return, SELF_METHOD_RETURN);

  CHECK_NE(type, EventProto::UNKNOWN);

  return type;
}

#undef CHECK_FIELD

}  // namespace peer
}  // namespace floating_temple
