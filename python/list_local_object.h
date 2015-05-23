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

#ifndef PYTHON_LIST_LOCAL_OBJECT_H_
#define PYTHON_LIST_LOCAL_OBJECT_H_

#include "third_party/Python-3.4.2/Include/Python.h"

#include "base/macros.h"
#include "python/local_object_impl.h"

namespace floating_temple {

class DeserializationContext;

namespace python {

class SequenceProto;

class ListLocalObject : public LocalObjectImpl {
 public:
  explicit ListLocalObject(PyObject* py_list_object);

  VersionedLocalObject* Clone() const override;
  std::string Dump() const override;

  static ListLocalObject* ParseListProto(const SequenceProto& list_proto,
                                         DeserializationContext* context);

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ListLocalObject);
};

}  // namespace python
}  // namespace floating_temple

#endif  // PYTHON_LIST_LOCAL_OBJECT_H_
