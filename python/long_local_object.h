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

#ifndef PYTHON_LONG_LOCAL_OBJECT_H_
#define PYTHON_LONG_LOCAL_OBJECT_H_

#include "third_party/Python-3.4.2/Include/Python.h"

#include "base/macros.h"
#include "python/local_object_impl.h"

namespace floating_temple {
namespace python {

class LongProto;

class LongLocalObject : public LocalObjectImpl {
 public:
  explicit LongLocalObject(PyObject* py_long_object);

  virtual LocalObject* Clone() const;
  virtual std::string Dump() const;

  static LongLocalObject* ParseLongProto(const LongProto& long_proto);

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;
  virtual bool InvokeTypeSpecificMethod(PeerObject* peer_object,
                                        const std::string& method_name,
                                        const std::vector<Value>& parameters,
                                        MethodContext* method_context,
                                        Value* return_value);

 private:
  PY_LONG_LONG GetLongLongValue(int* overflow) const;

  DISALLOW_COPY_AND_ASSIGN(LongLocalObject);
};

}  // namespace python
}  // namespace floating_temple

#endif  // PYTHON_LONG_LOCAL_OBJECT_H_
