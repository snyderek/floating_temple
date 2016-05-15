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

#include "toy_lang/zoo/expression_object.h"

#include <memory>
#include <string>
#include <vector>

#include "base/escape.h"
#include "base/logging.h"
#include "toy_lang/expression.h"
#include "toy_lang/proto/serialization.pb.h"
#include "util/dump_context.h"

using std::shared_ptr;
using std::string;
using std::vector;

namespace floating_temple {
namespace toy_lang {

ExpressionObject::ExpressionObject(
    const shared_ptr<const Expression>& expression)
    : expression_(expression) {
  CHECK(expression.get() != nullptr);
}

VersionedLocalObject* ExpressionObject::Clone() const {
  return new ExpressionObject(expression_);
}

void ExpressionObject::InvokeMethod(Thread* thread,
                                    ObjectReference* object_reference,
                                    const string& method_name,
                                    const vector<Value>& parameters,
                                    Value* return_value) {
  CHECK(return_value != nullptr);

  if (method_name == "eval") {
    CHECK_EQ(parameters.size(), 0u);

    ObjectReference* const object_reference = expression_->Evaluate(thread);
    if (object_reference == nullptr) {
      return;
    }
    return_value->set_object_reference(0, object_reference);
  } else {
    LOG(FATAL) << "Unsupported method: \"" << CEscape(method_name) << "\"";
  }
}

void ExpressionObject::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  // TODO(dss): Dump the contents of the expression.
  dc->BeginMap();
  dc->AddString("type");
  dc->AddString("ExpressionObject");
  dc->End();
}

// static
ExpressionObject* ExpressionObject::ParseExpressionProto(
    const ExpressionProto& expression_proto) {
  const shared_ptr<const Expression> expression(
      Expression::ParseExpressionProto(expression_proto));
  return new ExpressionObject(expression);
}

void ExpressionObject::PopulateObjectProto(
    ObjectProto* object_proto, SerializationContext* context) const {
  expression_->PopulateExpressionProto(
      object_proto->mutable_expression_object());
}

}  // namespace toy_lang
}  // namespace floating_temple
