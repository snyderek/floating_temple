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

#include "toy_lang/zoo/range_iterator_object.h"

#include <string>
#include <vector>

#include "base/escape.h"
#include "base/integral_types.h"
#include "base/logging.h"
#include "base/mutex.h"
#include "base/mutex_lock.h"
#include "toy_lang/proto/serialization.pb.h"
#include "toy_lang/wrap.h"
#include "util/dump_context.h"

using std::string;
using std::vector;

namespace floating_temple {
namespace toy_lang {

RangeIteratorObject::RangeIteratorObject(int64 limit, int64 start)
    : limit_(limit),
      i_(start) {
  CHECK_LE(start, limit);
}

LocalObject* RangeIteratorObject::Clone() const {
  int64 i_temp = 0;
  {
    MutexLock lock(&i_mu_);
    i_temp = i_;
  }

  return new RangeIteratorObject(limit_, i_temp);
}

void RangeIteratorObject::InvokeMethod(Thread* thread,
                                       ObjectReference* self_object_reference,
                                       const string& method_name,
                                       const vector<Value>& parameters,
                                       Value* return_value) {
  CHECK(return_value != nullptr);

  if (method_name == "has_next") {
    CHECK_EQ(parameters.size(), 0u);

    MutexLock lock(&i_mu_);
    CHECK_LE(i_, limit_);
    return_value->set_bool_value(0, i_ < limit_);
  } else if (method_name == "get_next") {
    CHECK_EQ(parameters.size(), 0u);

    MutexLock lock(&i_mu_);
    CHECK_LT(i_, limit_);
    return_value->set_object_reference(0, WrapInt(thread, i_));
    ++i_;
  } else {
    LOG(FATAL) << "Unsupported method: \"" << CEscape(method_name) << "\"";
  }
}

void RangeIteratorObject::Dump(DumpContext* dc) const {
  int64 i_temp = 0;
  {
    MutexLock lock(&i_mu_);
    i_temp = i_;
  }

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("RangeIteratorObject");

  dc->AddString("limit");
  dc->AddInt64(limit_);

  dc->AddString("i");
  dc->AddInt64(i_temp);

  dc->End();
}

// static
RangeIteratorObject* RangeIteratorObject::ParseRangeIteratorProto(
    const RangeIteratorProto& range_iterator_proto) {
  return new RangeIteratorObject(range_iterator_proto.limit(),
                                 range_iterator_proto.i());
}

void RangeIteratorObject::PopulateObjectProto(
    ObjectProto* object_proto, SerializationContext* context) const {
  RangeIteratorProto* const range_iterator_proto =
      object_proto->mutable_range_iterator_object();

  range_iterator_proto->set_limit(limit_);

  MutexLock lock(&i_mu_);
  range_iterator_proto->set_i(i_);
}

}  // namespace toy_lang
}  // namespace floating_temple
