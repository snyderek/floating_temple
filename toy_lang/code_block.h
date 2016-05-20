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

#ifndef TOY_LANG_CODE_BLOCK_H_
#define TOY_LANG_CODE_BLOCK_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/macros.h"

namespace floating_temple {

class DeserializationContext;
class ObjectReference;
class SerializationContext;
class Thread;

namespace toy_lang {

class CodeBlockProto;
class Expression;

// TODO(dss): Consider merging this class into the CodeBlockObject class.
class CodeBlock {
 public:
  CodeBlock(const std::shared_ptr<const Expression>& expression,
            const std::unordered_map<int, ObjectReference*>& external_symbols,
            const std::vector<int>& parameter_symbol_ids,
            const std::vector<int>& local_symbol_ids);
  ~CodeBlock();

  ObjectReference* Evaluate(const std::vector<ObjectReference*>& parameters,
                            Thread* thread) const;
  CodeBlock* Clone() const;
  void PopulateCodeBlockProto(CodeBlockProto* code_block_proto,
                              SerializationContext* context) const;
  std::string DebugString() const;

  static CodeBlock* ParseCodeBlockProto(const CodeBlockProto& code_block_proto,
                                        DeserializationContext* context);

 private:
  const std::shared_ptr<const Expression> expression_;
  const std::unordered_map<int, ObjectReference*> external_symbols_;
  const std::vector<int> parameter_symbol_ids_;
  const std::vector<int> local_symbol_ids_;

  DISALLOW_COPY_AND_ASSIGN(CodeBlock);
};

}  // namespace toy_lang
}  // namespace floating_temple

#endif  // TOY_LANG_CODE_BLOCK_H_
