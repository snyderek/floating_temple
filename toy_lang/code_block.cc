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
#include "include/c++/thread.h"
#include "toy_lang/expression.h"
#include "toy_lang/proto/serialization.pb.h"
#include "toy_lang/zoo/variable_object.h"

using std::pair;
using std::shared_ptr;
using std::string;
using std::unordered_map;
using std::vector;

namespace floating_temple {
namespace toy_lang {

CodeBlock::CodeBlock(
    const shared_ptr<const Expression>& expression,
    const unordered_map<int, ObjectReference*>& external_symbols,
    const vector<int>& parameter_symbol_ids,
    const vector<int>& local_symbol_ids)
    : expression_(expression),
      external_symbols_(external_symbols),
      parameter_symbol_ids_(parameter_symbol_ids),
      local_symbol_ids_(local_symbol_ids) {
  CHECK(expression.get() != nullptr);
}

CodeBlock::~CodeBlock() {
}

ObjectReference* CodeBlock::Evaluate(const vector<ObjectReference*>& parameters,
                                     Thread* thread) const {
  CHECK(thread != nullptr);

  const vector<ObjectReference*>::size_type parameter_count = parameters.size();
  CHECK_EQ(parameter_count, parameter_symbol_ids_.size());

  // Add the external symbol bindings.
  unordered_map<int, ObjectReference*> symbol_bindings(external_symbols_);

  // Add the parameter symbol bindings.
  for (vector<ObjectReference*>::size_type i = 0; i < parameter_count; ++i) {
    CHECK(symbol_bindings.emplace(parameter_symbol_ids_[i],
                                  parameters[i]).second);
  }

  // Create the local variables, all initially unset.
  for (int symbol_id : local_symbol_ids_) {
    VersionedLocalObject* const variable_object = new VariableObject(nullptr);
    ObjectReference* const object_reference = thread->CreateVersionedObject(
        variable_object, "");
    CHECK(symbol_bindings.emplace(symbol_id, object_reference).second);
  }

  return expression_->Evaluate(symbol_bindings, thread);
}

CodeBlock* CodeBlock::Clone() const {
  return new CodeBlock(expression_, external_symbols_, parameter_symbol_ids_,
                       local_symbol_ids_);
}

void CodeBlock::PopulateCodeBlockProto(CodeBlockProto* code_block_proto,
                                       SerializationContext* context) const {
  CHECK(code_block_proto != nullptr);
  CHECK(context != nullptr);

  expression_->PopulateExpressionProto(code_block_proto->mutable_expression());

  for (const pair<int, ObjectReference*>& external_symbol : external_symbols_) {
    ExternalSymbolProto* const external_symbol_proto =
        code_block_proto->add_external_symbol();
    external_symbol_proto->set_symbol_id(external_symbol.first);
    external_symbol_proto->set_object_index(
        context->GetIndexForObjectReference(external_symbol.second));
  }

  for (int symbol_id : parameter_symbol_ids_) {
    code_block_proto->add_parameter_symbol_id(symbol_id);
  }

  for (int symbol_id : local_symbol_ids_) {
    code_block_proto->add_local_symbol_id(symbol_id);
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

  const int external_symbol_count = code_block_proto.external_symbol_size();
  unordered_map<int, ObjectReference*> external_symbols;
  external_symbols.reserve(external_symbol_count);
  for (int i = 0; i < external_symbol_count; ++i) {
    const ExternalSymbolProto& external_symbol_proto =
        code_block_proto.external_symbol(i);
    const int symbol_id = external_symbol_proto.symbol_id();
    ObjectReference* const object_reference =
        context->GetObjectReferenceByIndex(external_symbol_proto.object_index());
    CHECK(external_symbols.emplace(symbol_id, object_reference).second);
  }

  const int parameter_symbol_count =
      code_block_proto.parameter_symbol_id_size();
  vector<int> parameter_symbol_ids(parameter_symbol_count);
  for (int i = 0; i < parameter_symbol_count; ++i) {
    parameter_symbol_ids[i] = code_block_proto.parameter_symbol_id(i);
  }

  const int local_symbol_count = code_block_proto.local_symbol_id_size();
  vector<int> local_symbol_ids(local_symbol_count);
  for (int i = 0; i < local_symbol_count; ++i) {
    local_symbol_ids[i] = code_block_proto.local_symbol_id(i);
  }

  return new CodeBlock(expression, external_symbols, parameter_symbol_ids,
                       local_symbol_ids);
}

}  // namespace toy_lang
}  // namespace floating_temple
