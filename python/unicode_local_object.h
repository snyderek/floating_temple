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

#ifndef PYTHON_UNICODE_LOCAL_OBJECT_H_
#define PYTHON_UNICODE_LOCAL_OBJECT_H_

#include "third_party/Python-3.4.2/Include/Python.h"

#include <string>

#include "base/macros.h"
#include "python/versioned_local_object_impl.h"

namespace floating_temple {
namespace python {

class UnicodeProto;

class UnicodeLocalObject : public VersionedLocalObjectImpl {
 public:
  explicit UnicodeLocalObject(PyObject* py_unicode);

  VersionedLocalObject* Clone() const override;
  void Dump(DumpContext* dc) const override;

  static UnicodeLocalObject* ParseUnicodeProto(
      const UnicodeProto& unicode_proto);

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(UnicodeLocalObject);
};

void SerializeUnicodeObject(PyObject* in, std::string* out);
PyObject* DeserializeUnicodeObject(const std::string& in);

}  // namespace python
}  // namespace floating_temple

#endif  // PYTHON_UNICODE_LOCAL_OBJECT_H_
