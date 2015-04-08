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

#ifndef PYTHON_LOCAL_OBJECT_IMPL_H_
#define PYTHON_LOCAL_OBJECT_IMPL_H_

#include "third_party/Python-3.4.2/Include/Python.h"

#include <cstddef>

#include "include/c++/local_object.h"

namespace floating_temple {

class DeserializationContext;
class SerializationContext;

namespace python {

class ObjectProto;

// Abstract class
class LocalObjectImpl : public LocalObject {
 public:
  // Steals a reference to 'py_object'.
  explicit LocalObjectImpl(PyObject* py_object);
  ~LocalObjectImpl() override;

  std::size_t Serialize(void* buffer, std::size_t buffer_size,
                        SerializationContext* context) const override;
  void InvokeMethod(Thread* thread,
                    PeerObject* peer_object,
                    const std::string& method_name,
                    const std::vector<Value>& parameters,
                    Value* return_value) override;

  static LocalObjectImpl* Deserialize(const void* buffer,
                                      std::size_t buffer_size,
                                      DeserializationContext* context);

 protected:
  PyObject* py_object() const { return py_object_; }

  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const = 0;

 private:
  PyObject* const py_object_;
};

}  // namespace python
}  // namespace floating_temple

#endif  // PYTHON_LOCAL_OBJECT_IMPL_H_
