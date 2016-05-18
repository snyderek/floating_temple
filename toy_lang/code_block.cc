// Floating Temple
// Copyright 2016 Derek S. Snyder
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

#include "toy_lang/code_block.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/string_printf.h"
#include "include/c++/deserialization_context.h"
#include "include/c++/serialization_context.h"
#include "toy_lang/expression.h"
#include "toy_lang/proto/serialization.pb.h"

using std::pair;
using std::shared_ptr;
using std::string;
using std::unordered_map;
using std::vector;

namespace floating_temple {
namespace toy_lang {

CodeBlock::CodeBlock(
    const shared_ptr<const Expression>& expression,
    const unordered_map<int, ObjectReference*>& symbol_bindings,
    const vector<int>& unbound_symbol_ids)
    : expression_(expression),
      symbol_bindings_(symbol_bindings),
      unbound_symbol_ids_(unbound_symbol_ids) {
  CHECK(expression.get() != nullptr);
}

CodeBlock::~CodeBlock() {
}

ObjectReference* CodeBlock::Evaluate(const vector<ObjectReference*>& parameters,
                                     Thread* thread) const {
  const vector<ObjectReference*>::size_type parameter_count = parameters.size();
  CHECK_EQ(parameter_count, unbound_symbol_ids_.size());

  unordered_map<int, ObjectReference*> all_symbol_bindings(symbol_bindings_);
  for (vector<ObjectReference*>::size_type i = 0; i < parameter_count; ++i) {
    CHECK(all_symbol_bindings.emplace(unbound_symbol_ids_[i],
                                      parameters[i]).second);
  }

  return expression_->Evaluate(all_symbol_bindings, thread);
}

CodeBlock* CodeBlock::Clone() const {
  return new CodeBlock(expression_, symbol_bindings_, unbound_symbol_ids_);
}

void CodeBlock::PopulateCodeBlockProto(CodeBlockProto* code_block_proto,
                                       SerializationContext* context) const {
  CHECK(code_block_proto != nullptr);
  CHECK(context != nullptr);

  expression_->PopulateExpressionProto(code_block_proto->mutable_expression());

  for (const pair<int, ObjectReference*>& symbol_binding : symbol_bindings_) {
    SymbolBindingProto* const symbol_binding_proto =
        code_block_proto->add_symbol_binding();
    symbol_binding_proto->set_symbol_id(symbol_binding.first);
    symbol_binding_proto->set_object_index(
        context->GetIndexForObjectReference(symbol_binding.second));
  }

  for (int symbol_id : unbound_symbol_ids_) {
    code_block_proto->add_unbound_symbol_id(symbol_id);
  }
}

string CodeBlock::DebugString() const {
  return StringPrintf("{%s}", expression_->DebugString().c_str());
}

// static
CodeBlock* CodeBlock::ParseCodeBlockProto(
    const CodeBlockProto& code_block_proto, DeserializationContext* context) {
  CHECK(context != nullptr);

  const shared_ptr<const Expression> expression(
      Expression::ParseExpressionProto(code_block_proto.expression()));

  const int symbol_binding_count = code_block_proto.symbol_binding_size();
  unordered_map<int, ObjectReference*> symbol_bindings;
  symbol_bindings.reserve(symbol_binding_count);
  for (int i = 0; i < symbol_binding_count; ++i) {
    const SymbolBindingProto& symbol_binding_proto =
        code_block_proto.symbol_binding(i);
    const int symbol_id = symbol_binding_proto.symbol_id();
    ObjectReference* const object_reference =
        context->GetObjectReferenceByIndex(symbol_binding_proto.object_index());
    CHECK(symbol_bindings.emplace(symbol_id, object_reference).second);
  }

  const int unbound_symbol_count = code_block_proto.unbound_symbol_id_size();
  vector<int> unbound_symbol_ids(unbound_symbol_count);
  for (int i = 0; i < unbound_symbol_count; ++i) {
    unbound_symbol_ids[i] = code_block_proto.unbound_symbol_id(i);
  }

  return new CodeBlock(expression, symbol_bindings, unbound_symbol_ids);
}

}  // namespace toy_lang
}  // namespace floating_temple
