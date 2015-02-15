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

#include <cstddef>
#include <string>
#include <tr1/cinttypes>
#include <vector>

#include "base/const_shared_ptr.h"
#include "base/escape.h"
#include "base/integral_types.h"
#include "base/linked_ptr.h"
#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "base/string_printf.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "toy_lang/get_serialized_expression_type.h"
#include "toy_lang/local_object_impl.h"
#include "toy_lang/proto/serialization.pb.h"
#include "toy_lang/symbol_table.h"

using std::string;
using std::vector;

namespace floating_temple {
namespace toy_lang {
namespace {

PeerObject* EvaluateExpressionList(
    PeerObject* symbol_table_object, Thread* thread,
    const vector<linked_ptr<Expression> >& expressions) {
  CHECK(thread != NULL);

  const vector<linked_ptr<Expression> >::size_type size = expressions.size();
  vector<PeerObject*> peer_objects(size);

  for (vector<linked_ptr<Expression> >::size_type i = 0; i < size; ++i) {
    PeerObject* const peer_object = expressions[i]->Evaluate(
        symbol_table_object, thread);

    if (peer_object == NULL) {
      return NULL;
    }

    peer_objects[i] = peer_object;
  }

  return thread->CreatePeerObject(new ListObject(peer_objects));
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

    case ExpressionProto::EXPRESSION:
      return ExpressionExpression::ParseExpressionExpressionProto(
          expression_proto.expression_expression());

    case ExpressionProto::VARIABLE:
      return VariableExpression::ParseVariableExpressionProto(
          expression_proto.variable_expression());

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

  return NULL;
}

IntExpression::IntExpression(int64 n)
    : n_(n) {
}

PeerObject* IntExpression::Evaluate(PeerObject* symbol_table_object,
                                    Thread* thread) const {
  CHECK(thread != NULL);
  return thread->CreatePeerObject(new IntObject(n_));
}

void IntExpression::PopulateExpressionProto(
    ExpressionProto* expression_proto) const {
  CHECK(expression_proto != NULL);
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

PeerObject* StringExpression::Evaluate(PeerObject* symbol_table_object,
                                       Thread* thread) const {
  CHECK(thread != NULL);
  return thread->CreatePeerObject(new StringObject(s_));
}

void StringExpression::PopulateExpressionProto(
    ExpressionProto* expression_proto) const {
  CHECK(expression_proto != NULL);
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

ExpressionExpression::ExpressionExpression(Expression* expression)
    : expression_(CHECK_NOTNULL(expression)) {
}

PeerObject* ExpressionExpression::Evaluate(PeerObject* symbol_table_object,
                                           Thread* thread) const {
  CHECK(thread != NULL);
  return thread->CreatePeerObject(new ExpressionObject(expression_));
}

void ExpressionExpression::PopulateExpressionProto(
    ExpressionProto* expression_proto) const {
  CHECK(expression_proto != NULL);

  expression_->PopulateExpressionProto(
      expression_proto->mutable_expression_expression()->mutable_expression());
}

string ExpressionExpression::DebugString() const {
  return StringPrintf("{%s}", expression_->DebugString().c_str());
}

// static
ExpressionExpression* ExpressionExpression::ParseExpressionExpressionProto(
    const ExpressionExpressionProto& expression_expression_proto) {
  Expression* const expression = Expression::ParseExpressionProto(
      expression_expression_proto.expression());
  return new ExpressionExpression(expression);
}

VariableExpression::VariableExpression(const string& name)
    : name_(name) {
  CHECK(!name.empty());
}

PeerObject* VariableExpression::Evaluate(PeerObject* symbol_table_object,
                                         Thread* thread) const {
  PeerObject* object = NULL;
  if (!GetVariable(symbol_table_object, thread, name_, &object)) {
    return NULL;
  }

  return object;
}

void VariableExpression::PopulateExpressionProto(
    ExpressionProto* expression_proto) const {
  CHECK(expression_proto != NULL);
  expression_proto->mutable_variable_expression()->set_name(name_);
}

string VariableExpression::DebugString() const {
  return name_;
}

// static
VariableExpression* VariableExpression::ParseVariableExpressionProto(
    const VariableExpressionProto& variable_expression_proto) {
  return new VariableExpression(variable_expression_proto.name());
}

FunctionExpression::FunctionExpression(Expression* function,
                                       const vector<Expression*>& parameters)
    : function_(CHECK_NOTNULL(function)),
      parameters_(parameters.size()) {
  for (vector<Expression*>::size_type i = 0; i < parameters.size(); ++i) {
    parameters_[i].reset(parameters[i]);
  }
}

PeerObject* FunctionExpression::Evaluate(PeerObject* symbol_table_object,
                                         Thread* thread) const {
  CHECK(thread != NULL);

  PeerObject* const function_object = function_->Evaluate(symbol_table_object,
                                                          thread);

  if (function_object == NULL) {
    return NULL;
  }

  PeerObject* const parameter_list_object = EvaluateExpressionList(
      symbol_table_object, thread, parameters_);

  if (parameter_list_object == NULL) {
    return NULL;
  }

  vector<Value> parameter_values(2);
  parameter_values[0].set_peer_object(0, symbol_table_object);
  parameter_values[1].set_peer_object(0, parameter_list_object);

  Value return_value;
  if (!thread->CallMethod(function_object, "call", parameter_values,
                          &return_value)) {
    return NULL;
  }

  CHECK_EQ(return_value.type(), Value::PEER_OBJECT)
      << "The 'call' method should have returned an object.";

  return return_value.peer_object();
}

void FunctionExpression::PopulateExpressionProto(
    ExpressionProto* expression_proto) const {
  CHECK(expression_proto != NULL);

  FunctionExpressionProto* const function_expression_proto =
      expression_proto->mutable_function_expression();

  function_->PopulateExpressionProto(
      function_expression_proto->mutable_function());

  for (vector<linked_ptr<Expression> >::const_iterator it = parameters_.begin();
       it != parameters_.end(); ++it) {
    (*it)->PopulateExpressionProto(function_expression_proto->add_parameter());
  }
}

string FunctionExpression::DebugString() const {
  string s;
  SStringPrintf(&s, "(%s", function_->DebugString().c_str());

  for (vector<linked_ptr<Expression> >::const_iterator it = parameters_.begin();
       it != parameters_.end(); ++it) {
    StringAppendF(&s, " %s", (*it)->DebugString().c_str());
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

PeerObject* ListExpression::Evaluate(PeerObject* symbol_table_object,
                                     Thread* thread) const {
  return EvaluateExpressionList(symbol_table_object, thread, list_items_);
}

void ListExpression::PopulateExpressionProto(
    ExpressionProto* expression_proto) const {
  CHECK(expression_proto != NULL);

  ListExpressionProto* const list_expression_proto =
      expression_proto->mutable_list_expression();

  for (vector<linked_ptr<Expression> >::const_iterator it = list_items_.begin();
       it != list_items_.end(); ++it) {
    (*it)->PopulateExpressionProto(list_expression_proto->add_list_item());
  }
}

string ListExpression::DebugString() const {
  string s = "[";

  for (vector<linked_ptr<Expression> >::const_iterator it = list_items_.begin();
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
