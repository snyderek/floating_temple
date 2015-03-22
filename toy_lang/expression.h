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

#ifndef TOY_LANG_EXPRESSION_H_
#define TOY_LANG_EXPRESSION_H_

#include <memory>
#include <string>
#include <vector>

#include "base/const_shared_ptr.h"
#include "base/integral_types.h"
#include "base/linked_ptr.h"
#include "base/macros.h"

namespace floating_temple {

class PeerObject;
class Thread;

namespace toy_lang {

class ExpressionExpressionProto;
class ExpressionProto;
class FunctionExpressionProto;
class IntExpressionProto;
class ListExpressionProto;
class StringExpressionProto;
class VariableExpressionProto;

class Expression {
 public:
  virtual ~Expression() {}

  virtual PeerObject* Evaluate(PeerObject* symbol_table_object,
                               Thread* thread) const = 0;
  virtual void PopulateExpressionProto(
      ExpressionProto* expression_proto) const = 0;

  virtual std::string DebugString() const = 0;

  static Expression* ParseExpressionProto(
      const ExpressionProto& expression_proto);
};

class IntExpression : public Expression {
 public:
  explicit IntExpression(int64 n);

  virtual PeerObject* Evaluate(PeerObject* symbol_table_object,
                               Thread* thread) const;
  virtual void PopulateExpressionProto(ExpressionProto* expression_proto) const;
  virtual std::string DebugString() const;

  static IntExpression* ParseIntExpressionProto(
      const IntExpressionProto& int_expression_proto);

 private:
  const int64 n_;

  DISALLOW_COPY_AND_ASSIGN(IntExpression);
};

class StringExpression : public Expression {
 public:
  explicit StringExpression(const std::string& s);

  virtual PeerObject* Evaluate(PeerObject* symbol_table_object,
                               Thread* thread) const;
  virtual void PopulateExpressionProto(ExpressionProto* expression_proto) const;
  virtual std::string DebugString() const;

  static StringExpression* ParseStringExpressionProto(
      const StringExpressionProto& string_expression_proto);

 private:
  const std::string s_;

  DISALLOW_COPY_AND_ASSIGN(StringExpression);
};

class ExpressionExpression : public Expression {
 public:
  explicit ExpressionExpression(Expression* expression);

  virtual PeerObject* Evaluate(PeerObject* symbol_table_object,
                               Thread* thread) const;
  virtual void PopulateExpressionProto(ExpressionProto* expression_proto) const;
  virtual std::string DebugString() const;

  static ExpressionExpression* ParseExpressionExpressionProto(
      const ExpressionExpressionProto& expression_expression_proto);

 private:
  const const_shared_ptr<Expression> expression_;

  DISALLOW_COPY_AND_ASSIGN(ExpressionExpression);
};

class VariableExpression : public Expression {
 public:
  explicit VariableExpression(const std::string& name);

  virtual PeerObject* Evaluate(PeerObject* symbol_table_object,
                               Thread* thread) const;
  virtual void PopulateExpressionProto(ExpressionProto* expression_proto) const;
  virtual std::string DebugString() const;

  static VariableExpression* ParseVariableExpressionProto(
      const VariableExpressionProto& variable_expression_proto);

 private:
  const std::string name_;

  DISALLOW_COPY_AND_ASSIGN(VariableExpression);
};

class FunctionExpression : public Expression {
 public:
  FunctionExpression(Expression* function,
                     const std::vector<Expression*>& parameters);

  virtual PeerObject* Evaluate(PeerObject* symbol_table_object,
                               Thread* thread) const;
  virtual void PopulateExpressionProto(ExpressionProto* expression_proto) const;
  virtual std::string DebugString() const;

  static FunctionExpression* ParseFunctionExpressionProto(
      const FunctionExpressionProto& function_expression_proto);

 private:
  const std::unique_ptr<Expression> function_;
  std::vector<linked_ptr<Expression>> parameters_;

  DISALLOW_COPY_AND_ASSIGN(FunctionExpression);
};

class ListExpression : public Expression {
 public:
  explicit ListExpression(const std::vector<Expression*>& list_items);

  virtual PeerObject* Evaluate(PeerObject* symbol_table_object,
                               Thread* thread) const;
  virtual void PopulateExpressionProto(ExpressionProto* expression_proto) const;
  virtual std::string DebugString() const;

  static ListExpression* ParseListExpressionProto(
      const ListExpressionProto& list_expression_proto);

 private:
  std::vector<linked_ptr<Expression>> list_items_;

  DISALLOW_COPY_AND_ASSIGN(ListExpression);
};

}  // namespace toy_lang
}  // namespace floating_temple

#endif  // TOY_LANG_EXPRESSION_H_
