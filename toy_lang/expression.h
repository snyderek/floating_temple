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
#include <unordered_map>
#include <vector>

#include "base/integral_types.h"
#include "base/macros.h"

namespace floating_temple {

class MethodContext;
class ObjectReference;

namespace toy_lang {

class BlockExpressionProto;
class ExpressionProto;
class FunctionCallExpressionProto;
class IntExpressionProto;
class ListExpressionProto;
class StringExpressionProto;
class SymbolExpressionProto;

class Expression {
 public:
  virtual ~Expression();

  virtual ObjectReference* Evaluate(
      const std::unordered_map<int, ObjectReference*>& symbol_bindings,
      MethodContext* method_context) const = 0;
  virtual void PopulateExpressionProto(
      ExpressionProto* expression_proto) const = 0;

  virtual std::string DebugString() const = 0;

  static Expression* ParseExpressionProto(
      const ExpressionProto& expression_proto);
};

class IntExpression : public Expression {
 public:
  explicit IntExpression(int64 n);

  ObjectReference* Evaluate(
      const std::unordered_map<int, ObjectReference*>& symbol_bindings,
      MethodContext* method_context) const override;
  void PopulateExpressionProto(
      ExpressionProto* expression_proto) const override;
  std::string DebugString() const override;

  static IntExpression* ParseIntExpressionProto(
      const IntExpressionProto& int_expression_proto);

 private:
  const int64 n_;

  DISALLOW_COPY_AND_ASSIGN(IntExpression);
};

class StringExpression : public Expression {
 public:
  explicit StringExpression(const std::string& s);

  ObjectReference* Evaluate(
      const std::unordered_map<int, ObjectReference*>& symbol_bindings,
      MethodContext* method_context) const override;
  void PopulateExpressionProto(
      ExpressionProto* expression_proto) const override;
  std::string DebugString() const override;

  static StringExpression* ParseStringExpressionProto(
      const StringExpressionProto& string_expression_proto);

 private:
  const std::string s_;

  DISALLOW_COPY_AND_ASSIGN(StringExpression);
};

class SymbolExpression : public Expression {
 public:
  explicit SymbolExpression(int symbol_id);

  ObjectReference* Evaluate(
      const std::unordered_map<int, ObjectReference*>& symbol_bindings,
      MethodContext* method_context) const override;
  void PopulateExpressionProto(
      ExpressionProto* expression_proto) const override;
  std::string DebugString() const override;

  static SymbolExpression* ParseSymbolExpressionProto(
      const SymbolExpressionProto& symbol_expression_proto);

 private:
  const int symbol_id_;

  DISALLOW_COPY_AND_ASSIGN(SymbolExpression);
};

class BlockExpression : public Expression {
 public:
  BlockExpression(const std::shared_ptr<const Expression>& expression,
                  const std::vector<int>& parameter_symbol_ids,
                  const std::vector<int>& local_symbol_ids);

  ObjectReference* Evaluate(
      const std::unordered_map<int, ObjectReference*>& symbol_bindings,
      MethodContext* method_context) const override;
  void PopulateExpressionProto(
      ExpressionProto* expression_proto) const override;
  std::string DebugString() const override;

  static BlockExpression* ParseBlockExpressionProto(
      const BlockExpressionProto& block_expression_proto);

 private:
  const std::shared_ptr<const Expression> expression_;
  const std::vector<int> parameter_symbol_ids_;
  const std::vector<int> local_symbol_ids_;

  DISALLOW_COPY_AND_ASSIGN(BlockExpression);
};

class FunctionCallExpression : public Expression {
 public:
  FunctionCallExpression(Expression* function,
                         const std::vector<Expression*>& parameters);

  ObjectReference* Evaluate(
      const std::unordered_map<int, ObjectReference*>& symbol_bindings,
      MethodContext* method_context) const override;
  void PopulateExpressionProto(
      ExpressionProto* expression_proto) const override;
  std::string DebugString() const override;

  static FunctionCallExpression* ParseFunctionCallExpressionProto(
      const FunctionCallExpressionProto& function_call_expression_proto);

 private:
  const std::unique_ptr<Expression> function_;
  std::vector<std::unique_ptr<Expression>> parameters_;

  DISALLOW_COPY_AND_ASSIGN(FunctionCallExpression);
};

class ListExpression : public Expression {
 public:
  explicit ListExpression(const std::vector<Expression*>& list_items);

  ObjectReference* Evaluate(
      const std::unordered_map<int, ObjectReference*>& symbol_bindings,
      MethodContext* method_context) const override;
  void PopulateExpressionProto(
      ExpressionProto* expression_proto) const override;
  std::string DebugString() const override;

  static ListExpression* ParseListExpressionProto(
      const ListExpressionProto& list_expression_proto);

 private:
  std::vector<std::unique_ptr<Expression>> list_items_;

  DISALLOW_COPY_AND_ASSIGN(ListExpression);
};

}  // namespace toy_lang
}  // namespace floating_temple

#endif  // TOY_LANG_EXPRESSION_H_
