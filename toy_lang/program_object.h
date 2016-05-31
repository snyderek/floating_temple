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

#ifndef TOY_LANG_PROGRAM_OBJECT_H_
#define TOY_LANG_PROGRAM_OBJECT_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "base/macros.h"
#include "include/c++/unversioned_local_object.h"

namespace floating_temple {

class ObjectReference;
class Thread;
class VersionedLocalObject;

namespace toy_lang {

class Expression;

class ProgramObject : public UnversionedLocalObject {
 public:
  // Takes ownership of 'expression'.
  ProgramObject(const std::unordered_map<std::string, int>& external_symbols,
                Expression* expression);
  ~ProgramObject() override;

  void InvokeMethod(Thread* thread,
                    ObjectReference* object_reference,
                    const std::string& method_name,
                    const std::vector<Value>& parameters,
                    Value* return_value) override;
  void Dump(DumpContext* dc) const override;

 private:
  void ResolveExternalSymbol(
      Thread* thread,
      const std::string& symbol_name,
      bool visible,
      VersionedLocalObject* local_object,
      std::unordered_map<int, ObjectReference*>* symbol_bindings) const;

  const std::unordered_map<std::string, int> external_symbols_;
  const std::unique_ptr<Expression> expression_;

  DISALLOW_COPY_AND_ASSIGN(ProgramObject);
};

}  // namespace toy_lang
}  // namespace floating_temple

#endif  // TOY_LANG_PROGRAM_OBJECT_H_
