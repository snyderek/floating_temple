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

#ifndef PYTHON_UNSERIALIZABLE_LOCAL_OBJECT_H_
#define PYTHON_UNSERIALIZABLE_LOCAL_OBJECT_H_

#include "third_party/Python-3.4.2/Include/Python.h"

#include "base/macros.h"
#include "python/local_object_impl.h"

namespace floating_temple {
namespace python {

class UnserializableLocalObject : public LocalObjectImpl {
 public:
  explicit UnserializableLocalObject(PyObject* py_object);

  VersionedLocalObject* Clone() const override;
  std::string Dump() const override;

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(UnserializableLocalObject);
};

}  // namespace python
}  // namespace floating_temple

#endif  // PYTHON_UNSERIALIZABLE_LOCAL_OBJECT_H_
