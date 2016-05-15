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

#include "toy_lang/expression.h"

#include <cinttypes>
#include <memory>
#include <string>
#include <vector>

#include "base/escape.h"
#include "base/integral_types.h"
#include "base/logging.h"
#include "base/string_printf.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "toy_lang/get_serialized_expression_type.h"
#include "toy_lang/proto/serialization.pb.h"
#include "toy_lang/zoo/expression_object.h"
#include "toy_lang/zoo/int_object.h"
#include "toy_lang/zoo/list_object.h"
#include "toy_lang/zoo/string_object.h"

using std::string;
using std::unique_ptr;
using std::vector;

namespace floating_temple {
namespace toy_lang {
namespace {

ObjectReference* EvaluateExpressionList(
    const vector<ObjectReference*>& symbol_bindings, Thread* thread,
    const vector<unique_ptr<Expression>>& expressions) {
  CHECK(thread != nullptr);

  const vector<unique_ptr<Expression>>::size_type size = expressions.size();
  vector<ObjectReference*> object_references(size);

  for (vector<unique_ptr<Expression>>::size_type i = 0; i < size; ++i) {
    ObjectReference* const object_reference = expressions[i]->Evaluate(
        symbol_bindings, thread);

    if (object_reference == nullptr) {
      return nullptr;
    }

    object_references[i] = object_reference;
  }

  return thread->CreateVersionedObject(new ListObject(object_references), "");
}

}  // namespace

// static
Expression* Expression::ParseExpressionProto(
    const ExpressionProto& expression_proto) {
  const ExpressionProto::Type expression_type = GetSerializedExpressionType(
      expression_proto);

  switch (expression_type) {
    case ExpressionProto::INT:
      return IntExpression::ParseIntExpressionProto(
          expression_proto.int_expression());

    case ExpressionProto::STRING:
      return StringExpression::ParseStringExpressionProto(
          expression_proto.string_expression());

    case ExpressionProto::BLOCK:
      return BlockExpression::ParseBlockExpressionProto(
          expression_proto.block_expression());

    case ExpressionProto::FUNCTION:
      return FunctionExpression::ParseFunctionExpressionProto(
          expression_proto.function_expression());

    case ExpressionProto::LIST:
      return ListExpression::ParseListExpressionProto(
          expression_proto.list_expression());

    default:
      LOG(FATAL) << "Unexpected expression type: "
                 << static_cast<int>(expression_type);
  }

  return nullptr;
}

IntExpression::IntExpression(int64 n)
    : n_(n) {
}

ObjectReference* IntExpression::Evaluate(
    const vector<ObjectReference*>& symbol_bindings, Thread* thread) const {
  CHECK(thread != nullptr);
  return thread->CreateVersionedObject(new IntObject(n_), "");
}

void IntExpression::PopulateExpressionProto(
    ExpressionProto* expression_proto) const {
  CHECK(expression_proto != nullptr);
  expression_proto->mutable_int_expression()->set_int_value(n_);
}

string IntExpression::DebugString() const {
  return StringPrintf("%" PRId64, n_);
}

// static
IntExpression* IntExpression::ParseIntExpressionProto(
    const IntExpressionProto& int_expression_proto) {
  return new IntExpression(int_expression_proto.int_value());
}

StringExpression::StringExpression(const string& s)
    : s_(s) {
}

ObjectReference* StringExpression::Evaluate(
    const vector<ObjectReference*>& symbol_bindings, Thread* thread) const {
  CHECK(thread != nullptr);
  return thread->CreateVersionedObject(new StringObject(s_), "");
}

void StringExpression::PopulateExpressionProto(
    ExpressionProto* expression_proto) const {
  CHECK(expression_proto != nullptr);
  expression_proto->mutable_string_expression()->set_string_value(s_);
}

string StringExpression::DebugString() const {
  return StringPrintf("\"%s\"", CEscape(s_).c_str());
}

// static
StringExpression* StringExpression::ParseStringExpressionProto(
    const StringExpressionProto& string_expression_proto) {
  return new StringExpression(string_expression_proto.string_value());
}

SymbolExpression::SymbolExpression(int symbol_id)
    : symbol_id_(symbol_id) {
  CHECK_GE(symbol_id, 0);
}

ObjectReference* SymbolExpression::Evaluate(
    const vector<ObjectReference*>& symbol_bindings, Thread* thread) const {
  const vector<ObjectReference*>::size_type index =
      static_cast<vector<ObjectReference*>::size_type>(symbol_id_);
  CHECK_LT(index, symbol_bindings.size());

  return symbol_bindings[index];
}

void SymbolExpression::PopulateExpressionProto(
    ExpressionProto* expression_proto) const {
  CHECK(expression_proto != nullptr);
  expression_proto->mutable_symbol_expression()->set_symbol_id(symbol_id_);
}

string SymbolExpression::DebugString() const {
  return StringPrintf("@%d", symbol_id_);
}

// static
SymbolExpression* SymbolExpression::ParseSymbolExpressionProto(
    const SymbolExpressionProto& symbol_expression_proto) {
  return new SymbolExpression(symbol_expression_proto.symbol_id());
}

BlockExpression::BlockExpression(Expression* expression, int bound_symbol_count,
                                 int unbound_symbol_count)
    : expression_(CHECK_NOTNULL(expression)),
      bound_symbol_count_(bound_symbol_count),
      unbound_symbol_count_(unbound_symbol_count) {
  CHECK_GE(bound_symbol_count, 0);
  CHECK_GE(unbound_symbol_count, 0);
}

ObjectReference* BlockExpression::Evaluate(
    const vector<ObjectReference*>& symbol_bindings, Thread* thread) const {
  CHECK(thread != nullptr);

  VersionedLocalObject* const expression_object = new ExpressionObject(
      expression_, bound_symbol_count_, unbound_symbol_count_);
  return thread->CreateVersionedObject(expression_object, "");
}

void BlockExpression::PopulateExpressionProto(
    ExpressionProto* expression_proto) const {
  CHECK(expression_proto != nullptr);

  BlockExpressionProto* const block_expression_proto =
      expression_proto->mutable_block_expression();
  expression_->PopulateExpressionProto(
      block_expression_proto->mutable_expression());
  block_expression_proto->set_bound_symbol_count(bound_symbol_count_);
  block_expression_proto->set_unbound_symbol_count(unbound_symbol_count_);
}

string BlockExpression::DebugString() const {
  return StringPrintf("{%s}", expression_->DebugString().c_str());
}

// static
BlockExpression* BlockExpression::ParseBlockExpressionProto(
    const BlockExpressionProto& block_expression_proto) {
  Expression* const expression = Expression::ParseExpressionProto(
      block_expression_proto.expression());
  const int bound_symbol_count = block_expression_proto.bound_symbol_count();
  const int unbound_symbol_count =
      block_expression_proto.unbound_symbol_count();

  return new BlockExpression(expression, bound_symbol_count,
                             unbound_symbol_count);
}

FunctionExpression::FunctionExpression(Expression* function,
                                       const vector<Expression*>& parameters)
    : function_(CHECK_NOTNULL(function)),
      parameters_(parameters.size()) {
  for (vector<Expression*>::size_type i = 0; i < parameters.size(); ++i) {
    parameters_[i].reset(parameters[i]);
  }
}

ObjectReference* FunctionExpression::Evaluate(
    const vector<ObjectReference*>& symbol_bindings, Thread* thread) const {
  CHECK(thread != nullptr);

  ObjectReference* const function_object = function_->Evaluate(symbol_bindings,
                                                               thread);

  if (function_object == nullptr) {
    return nullptr;
  }

  ObjectReference* const parameter_list_object = EvaluateExpressionList(
      symbol_bindings, thread, parameters_);

  if (parameter_list_object == nullptr) {
    return nullptr;
  }

  vector<Value> parameter_values(1);
  parameter_values[1].set_object_reference(0, parameter_list_object);

  Value return_value;
  if (!thread->CallMethod(function_object, "call", parameter_values,
                          &return_value)) {
    return nullptr;
  }

  CHECK_EQ(return_value.type(), Value::OBJECT_REFERENCE)
      << "The 'call' method should have returned an object.";

  return return_value.object_reference();
}

void FunctionExpression::PopulateExpressionProto(
    ExpressionProto* expression_proto) const {
  CHECK(expression_proto != nullptr);

  FunctionExpressionProto* const function_expression_proto =
      expression_proto->mutable_function_expression();

  function_->PopulateExpressionProto(
      function_expression_proto->mutable_function());

  for (const unique_ptr<Expression>& parameter : parameters_) {
    parameter->PopulateExpressionProto(
        function_expression_proto->add_parameter());
  }
}

string FunctionExpression::DebugString() const {
  string s;
  SStringPrintf(&s, "(%s", function_->DebugString().c_str());

  for (const unique_ptr<Expression>& parameter : parameters_) {
    StringAppendF(&s, " %s", parameter->DebugString().c_str());
  }

  s += ')';

  return s;
}

// static
FunctionExpression* FunctionExpression::ParseFunctionExpressionProto(
    const FunctionExpressionProto& function_expression_proto) {
  Expression* const function = Expression::ParseExpressionProto(
      function_expression_proto.function());

  vector<Expression*> parameters(function_expression_proto.parameter_size());
  for (int i = 0; i < function_expression_proto.parameter_size(); ++i) {
    parameters[i] = Expression::ParseExpressionProto(
        function_expression_proto.parameter(i));
  }

  return new FunctionExpression(function, parameters);
}

ListExpression::ListExpression(const vector<Expression*>& list_items)
    : list_items_(list_items.size()) {
  for (vector<Expression*>::size_type i = 0; i < list_items.size(); ++i) {
    list_items_[i].reset(list_items[i]);
  }
}

ObjectReference* ListExpression::Evaluate(
    const vector<ObjectReference*>& symbol_bindings, Thread* thread) const {
  return EvaluateExpressionList(symbol_bindings, thread, list_items_);
}

void ListExpression::PopulateExpressionProto(
    ExpressionProto* expression_proto) const {
  CHECK(expression_proto != nullptr);

  ListExpressionProto* const list_expression_proto =
      expression_proto->mutable_list_expression();

  for (const unique_ptr<Expression>& list_item : list_items_) {
    list_item->PopulateExpressionProto(list_expression_proto->add_list_item());
  }
}

string ListExpression::DebugString() const {
  string s = "[";

  for (vector<unique_ptr<Expression>>::const_iterator it = list_items_.begin();
       it != list_items_.end(); ++it) {
    if (it != list_items_.begin()) {
      s += ' ';
    }

    StringAppendF(&s, "%s", (*it)->DebugString().c_str());
  }

  s += ']';

  return s;
}

// static
ListExpression* ListExpression::ParseListExpressionProto(
    const ListExpressionProto& list_expression_proto) {
  vector<Expression*> list_items(list_expression_proto.list_item_size());
  for (int i = 0; i < list_expression_proto.list_item_size(); ++i) {
    list_items[i] = Expression::ParseExpressionProto(
        list_expression_proto.list_item(i));
  }

  return new ListExpression(list_items);
}

}  // namespace toy_lang
}  // namespace floating_temple
