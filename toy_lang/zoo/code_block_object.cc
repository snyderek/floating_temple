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

#include "toy_lang/zoo/code_block_object.h"

#include <memory>
#include <string>
#include <vector>

#include "base/escape.h"
#include "base/integral_types.h"
#include "base/logging.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "toy_lang/code_block.h"
#include "toy_lang/proto/serialization.pb.h"
#include "util/dump_context.h"

using std::string;
using std::vector;

namespace floating_temple {

class ObjectReference;

namespace toy_lang {

CodeBlockObject::CodeBlockObject(CodeBlock* code_block)
    : code_block_(CHECK_NOTNULL(code_block)) {
}

CodeBlockObject::~CodeBlockObject() {
}

LocalObject* CodeBlockObject::Clone() const {
  return new CodeBlockObject(code_block_->Clone());
}

void CodeBlockObject::InvokeMethod(Thread* thread,
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
    vector<ObjectReference*> parameter_object_references(parameter_count);

    for (int64 i = 0; i < parameter_count; ++i) {
      vector<Value> get_at_params(1);
      get_at_params[0].set_int64_value(0, i);

      Value list_item_value;
      if (!thread->CallMethod(parameter_list_object, "get_at", get_at_params,
                              &list_item_value)) {
        return;
      }

      parameter_object_references[i] = list_item_value.object_reference();
    }

    ObjectReference* const object_reference = code_block_->Evaluate(
        parameter_object_references, thread);
    if (object_reference == nullptr) {
      return;
    }
    return_value->set_object_reference(0, object_reference);
  } else {
    LOG(FATAL) << "Unsupported method: \"" << CEscape(method_name) << "\"";
  }
}

void CodeBlockObject::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  dc->BeginMap();

  dc->AddString("type");
  dc->AddString("CodeBlockObject");

  dc->AddString("code_block");
  dc->AddString(code_block_->DebugString());

  dc->End();
}

// static
CodeBlockObject* CodeBlockObject::ParseCodeBlockObjectProto(
    const CodeBlockObjectProto& code_block_object_proto,
    DeserializationContext* context) {
  CodeBlock* const code_block = CodeBlock::ParseCodeBlockProto(
      code_block_object_proto.code_block(), context);

  return new CodeBlockObject(code_block);
}

void CodeBlockObject::PopulateObjectProto(ObjectProto* object_proto,
                                          SerializationContext* context) const {
  CodeBlockObjectProto* const code_block_object_proto =
      object_proto->mutable_code_block_object();

  code_block_->PopulateCodeBlockProto(
      code_block_object_proto->mutable_code_block(), context);
}

}  // namespace toy_lang
}  // namespace floating_temple
