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

#include "toy_lang/zoo/list_object.h"

#include <string>
#include <vector>

#include "base/escape.h"
#include "base/integral_types.h"
#include "base/logging.h"
#include "base/mutex.h"
#include "base/mutex_lock.h"
#include "include/c++/deserialization_context.h"
#include "include/c++/object_reference.h"
#include "include/c++/serialization_context.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "toy_lang/proto/serialization.pb.h"
#include "toy_lang/wrap.h"
#include "util/dump_context.h"

using std::string;
using std::vector;

namespace floating_temple {
namespace toy_lang {
namespace {

int64 TrueMod(int64 a, int64 b) {
  CHECK_NE(b, 0);
  return (a % b + b) % b;
}

}  // namespace

ListObject::ListObject(const vector<ObjectReference*>& items)
    : items_(items) {
}

LocalObject* ListObject::Clone() const {
  MutexLock lock(&items_mu_);
  return new ListObject(items_);
}

void ListObject::InvokeMethod(Thread* thread,
                              ObjectReference* self_object_reference,
                              const string& method_name,
                              const vector<Value>& parameters,
                              Value* return_value) {
  CHECK(return_value != nullptr);

  if (method_name == "length") {
    CHECK_EQ(parameters.size(), 0u);

    MutexLock lock(&items_mu_);
    return_value->set_int64_value(0, items_.size());
  } else if (method_name == "get_at") {
    CHECK_EQ(parameters.size(), 1u);

    const int64 index = parameters[0].int64_value();

    MutexLock lock(&items_mu_);
    CHECK(!items_.empty());
    const int64 length = static_cast<int64>(items_.size());
    return_value->set_object_reference(0, items_[TrueMod(index, length)]);
  } else if (method_name == "append") {
    CHECK_EQ(parameters.size(), 1u);

    {
      MutexLock lock(&items_mu_);
      items_.push_back(parameters[0].object_reference());
    }

    return_value->set_empty(0);
  } else if (method_name == "get_string") {
    CHECK_EQ(parameters.size(), 0u);

    string s = "[";
    {
      MutexLock lock(&items_mu_);

      for (vector<ObjectReference*>::const_iterator it = items_.begin();
           it != items_.end(); ++it) {
        if (it != items_.begin()) {
          s += ' ';
        }

        string item_str;
        if (!UnwrapString(thread, *it, &item_str)) {
          return;
        }

        s += item_str;
      }
    }
    s += ']';

    return_value->set_string_value(0, s);
  } else {
    LOG(FATAL) << "Unsupported method: \"" << CEscape(method_name) << "\"";
  }
}

void ListObject::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  vector<ObjectReference*> items_temp;
  {
    MutexLock lock(&items_mu_);
    items_temp = items_;
  }

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("ListObject");

  dc->AddString("items");
  dc->BeginList();
  for (const ObjectReference* const peer_object : items_temp) {
    peer_object->Dump(dc);
  }
  dc->End();

  dc->End();
}

// static
ListObject* ListObject::ParseListProto(const ListProto& list_proto,
                                       DeserializationContext* context) {
  CHECK(context != nullptr);

  vector<ObjectReference*> items(list_proto.object_index_size());

  for (int i = 0; i < list_proto.object_index_size(); ++i) {
    const int64 object_index = list_proto.object_index(i);
    items[i] = context->GetObjectReferenceByIndex(object_index);
  }

  return new ListObject(items);
}

void ListObject::PopulateObjectProto(ObjectProto* object_proto,
                                     SerializationContext* context) const {
  ListProto* const list_proto = object_proto->mutable_list_object();

  MutexLock lock(&items_mu_);

  for (ObjectReference* const object_reference : items_) {
    const int object_index = context->GetIndexForObjectReference(
        object_reference);
    list_proto->add_object_index(object_index);
  }
}

}  // namespace toy_lang
}  // namespace floating_temple
