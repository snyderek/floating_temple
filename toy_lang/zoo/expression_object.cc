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
#include "base/integral_types.h"
#include "base/logging.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "toy_lang/expression.h"
#include "toy_lang/proto/serialization.pb.h"
#include "util/dump_context.h"

using std::shared_ptr;
using std::string;
using std::vector;

namespace floating_temple {

class ObjectReference;

namespace toy_lang {

ExpressionObject::ExpressionObject(
    const shared_ptr<const Expression>& expression, int bound_symbol_count,
    int unbound_symbol_count)
    : expression_(expression),
      bound_symbol_count_(bound_symbol_count),
      unbound_symbol_count_(unbound_symbol_count) {
  CHECK(expression.get() != nullptr);
  CHECK_GE(bound_symbol_count, 0);
  CHECK_GE(unbound_symbol_count, 0);
}

VersionedLocalObject* ExpressionObject::Clone() const {
  return new ExpressionObject(expression_, bound_symbol_count_,
                              unbound_symbol_count_);
}

void ExpressionObject::InvokeMethod(Thread* thread,
                                    ObjectReference* object_reference,
                                    const string& method_name,
                                    const vector<Value>& parameters,
                                    Value* return_value) {
  CHECK(return_value != nullptr);

  if (method_name == "eval") {
    CHECK_EQ(parameters.size(), 1u);

    ObjectReference* const parameter_list_object =
        parameters[0].object_reference();

    Value length_value;
    if (!thread->CallMethod(parameter_list_object, "length", vector<Value>(),
                            &length_value)) {
      return;
    }

    const int64 parameter_count = length_value.int64_value();
    vector<ObjectReference*> symbol_bindings(parameter_count);

    for (int64 i = 0; i < parameter_count; ++i) {
      vector<Value> get_at_params(1);
      get_at_params[0].set_int64_value(0, i);

      Value list_item_value;
      if (!thread->CallMethod(parameter_list_object, "get_at", get_at_params,
                              &list_item_value)) {
        return;
      }

      symbol_bindings[i] = list_item_value.object_reference();
    }

    ObjectReference* const object_reference = expression_->Evaluate(
        symbol_bindings, thread);
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

  dc->BeginMap();
  dc->AddString("type");
  dc->AddString("ExpressionObject");

  // TODO(dss): Dump the contents of the expression.

  dc->AddString("bound_symbol_count");
  dc->AddInt(bound_symbol_count_);

  dc->AddString("unbound_symbol_count");
  dc->AddInt(unbound_symbol_count_);

  dc->End();
}

// static
ExpressionObject* ExpressionObject::ParseExpressionProto(
    const ExpressionObjectProto& expression_object_proto) {
  const shared_ptr<const Expression> expression(
      Expression::ParseExpressionProto(expression_object_proto.expression()));
  const int bound_symbol_count = expression_object_proto.bound_symbol_count();
  const int unbound_symbol_count =
      expression_object_proto.unbound_symbol_count();

  return new ExpressionObject(expression, bound_symbol_count,
                              unbound_symbol_count);
}

void ExpressionObject::PopulateObjectProto(
    ObjectProto* object_proto, SerializationContext* context) const {
  ExpressionObjectProto* const expression_object_proto =
      object_proto->mutable_expression_object();

  expression_->PopulateExpressionProto(
      expression_object_proto->mutable_expression());
  expression_object_proto->set_bound_symbol_count(bound_symbol_count_);
  expression_object_proto->set_unbound_symbol_count(unbound_symbol_count_);
}

}  // namespace toy_lang
}  // namespace floating_temple
