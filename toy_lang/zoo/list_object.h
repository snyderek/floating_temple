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

#ifndef TOY_LANG_ZOO_LIST_OBJECT_H_
#define TOY_LANG_ZOO_LIST_OBJECT_H_

#include <vector>

#include "base/macros.h"
#include "base/mutex.h"
#include "toy_lang/zoo/local_object_impl.h"

namespace floating_temple {

class DeserializationContext;
class ObjectReference;

namespace toy_lang {

class ListProto;

class ListObject : public LocalObjectImpl {
 public:
  explicit ListObject(const std::vector<ObjectReference*>& items);

  LocalObject* Clone() const override;
  void InvokeMethod(Thread* thread,
                    ObjectReference* object_reference,
                    const std::string& method_name,
                    const std::vector<Value>& parameters,
                    Value* return_value) override;
  void Dump(DumpContext* dc) const override;

  static ListObject* ParseListProto(const ListProto& list_proto,
                                    DeserializationContext* context);

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;

 private:
  std::vector<ObjectReference*> items_;
  mutable Mutex items_mu_;

  DISALLOW_COPY_AND_ASSIGN(ListObject);
};

}  // namespace toy_lang
}  // namespace floating_temple

#endif  // TOY_LANG_ZOO_LIST_OBJECT_H_
