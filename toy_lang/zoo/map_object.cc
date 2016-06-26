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

#include "toy_lang/zoo/map_object.h"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/escape.h"
#include "base/integral_types.h"
#include "base/logging.h"
#include "include/c++/deserialization_context.h"
#include "include/c++/object_reference.h"
#include "include/c++/serialization_context.h"
#include "toy_lang/proto/serialization.pb.h"
#include "util/dump_context.h"

using std::pair;
using std::string;
using std::unordered_map;
using std::vector;

namespace floating_temple {
namespace toy_lang {

MapObject::MapObject() {
}

LocalObject* MapObject::Clone() const {
  MapObject* new_object = new MapObject();
  new_object->map_ = map_;

  return new_object;
}

void MapObject::InvokeMethod(MethodContext* method_context,
                             ObjectReference* self_object_reference,
                             const string& method_name,
                             const vector<Value>& parameters,
                             Value* return_value) {
  CHECK(return_value != nullptr);

  if (method_name == "is_set") {
    CHECK_EQ(parameters.size(), 1u);

    const string& key = parameters[0].string_value();
    return_value->set_bool_value(0, map_.find(key) != map_.end());
  } else if (method_name == "get") {
    CHECK_EQ(parameters.size(), 1u);

    const string& key = parameters[0].string_value();

    const unordered_map<string, ObjectReference*>::const_iterator it =
        map_.find(key);
    CHECK(it != map_.end()) << "Key not found: \"" << CEscape(key) << "\"";

    return_value->set_object_reference(0, it->second);
  } else if (method_name == "set") {
    CHECK_EQ(parameters.size(), 2u);

    const string& key = parameters[0].string_value();
    ObjectReference* const object_reference = parameters[1].object_reference();

    map_[key] = object_reference;

    return_value->set_empty(0);
  } else {
    LOG(FATAL) << "Unsupported method: \"" << CEscape(method_name) << "\"";
  }
}

void MapObject::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("MapObject");

  dc->AddString("map");
  dc->BeginMap();
  for (const pair<string, ObjectReference*>& map_pair : map_) {
    dc->AddString(map_pair.first);
    map_pair.second->Dump(dc);
  }
  dc->End();

  dc->End();
}

// static
MapObject* MapObject::ParseMapProto(const MapProto& map_proto,
                                    DeserializationContext* context) {
  CHECK(context != nullptr);

  MapObject* const new_object = new MapObject();
  unordered_map<string, ObjectReference*>* const the_map = &new_object->map_;

  for (int i = 0; i < map_proto.entry_size(); ++i) {
    const MapEntryProto& entry_proto = map_proto.entry(i);

    const string& key = entry_proto.key();
    const int64 object_index = entry_proto.value_object_index();
    ObjectReference* const object_reference =
        context->GetObjectReferenceByIndex(object_index);

    CHECK(the_map->emplace(key, object_reference).second);
  }

  return new_object;
}

void MapObject::PopulateObjectProto(ObjectProto* object_proto,
                                    SerializationContext* context) const {
  CHECK(context != nullptr);

  MapProto* const map_proto = object_proto->mutable_map_object();

  for (const auto& map_pair : map_) {
    MapEntryProto* const entry_proto = map_proto->add_entry();

    ObjectReference* const object_reference = map_pair.second;
    const int object_index = context->GetIndexForObjectReference(
        object_reference);

    entry_proto->set_key(map_pair.first);
    entry_proto->set_value_object_index(object_index);
  }
}

}  // namespace toy_lang
}  // namespace floating_temple
