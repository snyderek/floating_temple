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
#include <unordered_map>
#include <vector>

#include "base/escape.h"
#include "base/integral_types.h"
#include "base/logging.h"
#include "base/string_printf.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "toy_lang/code_block.h"
#include "toy_lang/get_serialized_expression_type.h"
#include "toy_lang/proto/serialization.pb.h"
#include "toy_lang/zoo/expression_object.h"
#include "toy_lang/zoo/int_object.h"
#include "toy_lang/zoo/list_object.h"
#include "toy_lang/zoo/string_object.h"

using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;

namespace floating_temple {
namespace toy_lang {
namespace {

ObjectReference* EvaluateExpressionList(
    const unordered_map<int, ObjectReference*>& symbol_bindings, Thread* thread,
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

Expression::~Expression() {
}

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

    case ExpressionProto::FUNCTION_CALL:
      return FunctionCallExpression::ParseFunctionCallExpressionProto(
          expression_proto.function_call_expression());

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
    const unordered_map<int, ObjectReference*>& symbol_bindings,
    Thread* thread) const {
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
    const unordered_map<int, ObjectReference*>& symbol_bindings,
    Thread* thread) const {
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
    const unordered_map<int, ObjectReference*>& symbol_bindings,
    Thread* thread) const {
  const auto it = symbol_bindings.find(symbol_id_);
  CHECK(it != symbol_bindings.end());
  return it->second;
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

BlockExpression::BlockExpression(const shared_ptr<const Expression>& expression,
                                 const vector<int>& unbound_symbol_ids)
    : expression_(expression),
      unbound_symbol_ids_(unbound_symbol_ids) {
  CHECK(expression.get() != nullptr);
}

ObjectReference* BlockExpression::Evaluate(
    const unordered_map<int, ObjectReference*>& symbol_bindings,
    Thread* thread) const {
  CHECK(thread != nullptr);

  // TODO(dss): Set the 'local_symbol_ids' parameter.
  CodeBlock* const code_block = new CodeBlock(expression_, symbol_bindings,
                                              unbound_symbol_ids_,
                                              vector<int>());
  VersionedLocalObject* const expression_object = new ExpressionObject(
      code_block);
  return thread->CreateVersionedObject(expression_object, "");
}

void BlockExpression::PopulateExpressionProto(
    ExpressionProto* expression_proto) const {
  CHECK(expression_proto != nullptr);

  BlockExpressionProto* const block_expression_proto =
      expression_proto->mutable_block_expression();
  expression_->PopulateExpressionProto(
      block_expression_proto->mutable_expression());
  for (int symbol_id : unbound_symbol_ids_) {
    block_expression_proto->add_unbound_symbol_id(symbol_id);
  }
}

string BlockExpression::DebugString() const {
  return StringPrintf("{%s}", expression_->DebugString().c_str());
}

// static
BlockExpression* BlockExpression::ParseBlockExpressionProto(
    const BlockExpressionProto& block_expression_proto) {
  const shared_ptr<const Expression> expression(
      Expression::ParseExpressionProto(block_expression_proto.expression()));

  const int unbound_symbol_count =
      block_expression_proto.unbound_symbol_id_size();
  vector<int> unbound_symbol_ids(unbound_symbol_count);
  for (int i = 0; i < unbound_symbol_count; ++i) {
    unbound_symbol_ids[i] = block_expression_proto.unbound_symbol_id(i);
  }

  return new BlockExpression(expression, unbound_symbol_ids);
}

FunctionCallExpression::FunctionCallExpression(
    Expression* function, const vector<Expression*>& parameters)
    : function_(CHECK_NOTNULL(function)),
      parameters_(parameters.size()) {
  for (vector<Expression*>::size_type i = 0; i < parameters.size(); ++i) {
    parameters_[i].reset(parameters[i]);
  }
}

ObjectReference* FunctionCallExpression::Evaluate(
    const unordered_map<int, ObjectReference*>& symbol_bindings,
    Thread* thread) const {
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

void FunctionCallExpression::PopulateExpressionProto(
    ExpressionProto* expression_proto) const {
  CHECK(expression_proto != nullptr);

  FunctionCallExpressionProto* const function_call_expression_proto =
      expression_proto->mutable_function_call_expression();

  function_->PopulateExpressionProto(
      function_call_expression_proto->mutable_function());

  for (const unique_ptr<Expression>& parameter : parameters_) {
    parameter->PopulateExpressionProto(
        function_call_expression_proto->add_parameter());
  }
}

string FunctionCallExpression::DebugString() const {
  string s;
  SStringPrintf(&s, "(%s", function_->DebugString().c_str());

  for (const unique_ptr<Expression>& parameter : parameters_) {
    StringAppendF(&s, " %s", parameter->DebugString().c_str());
  }

  s += ')';

  return s;
}

// static
FunctionCallExpression*
FunctionCallExpression::ParseFunctionCallExpressionProto(
    const FunctionCallExpressionProto& function_call_expression_proto) {
  Expression* const function = Expression::ParseExpressionProto(
      function_call_expression_proto.function());

  vector<Expression*> parameters(
      function_call_expression_proto.parameter_size());
  for (int i = 0; i < function_call_expression_proto.parameter_size(); ++i) {
    parameters[i] = Expression::ParseExpressionProto(
        function_call_expression_proto.parameter(i));
  }

  return new FunctionCallExpression(function, parameters);
}

ListExpression::ListExpression(const vector<Expression*>& list_items)
    : list_items_(list_items.size()) {
  for (vector<Expression*>::size_type i = 0; i < list_items.size(); ++i) {
    list_items_[i].reset(list_items[i]);
  }
}

ObjectReference* ListExpression::Evaluate(
    const unordered_map<int, ObjectReference*>& symbol_bindings,
    Thread* thread) const {
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
