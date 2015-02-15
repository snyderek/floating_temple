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

#ifndef PYTHON_FALSE_LOCAL_OBJECT_H_
#define PYTHON_FALSE_LOCAL_OBJECT_H_

#include "base/macros.h"
#include "python/local_object_impl.h"

namespace floating_temple {
namespace python {

// Local object that wraps the Python 'False' object.
class FalseLocalObject : public LocalObjectImpl {
 public:
  FalseLocalObject();

  virtual LocalObject* Clone() const;
  virtual std::string Dump() const;

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;
  virtual bool InvokeTypeSpecificMethod(PeerObject* peer_object,
                                        const std::string& method_name,
                                        const std::vector<Value>& parameters,
                                        MethodContext* method_context,
                                        Value* return_value);

 private:
  DISALLOW_COPY_AND_ASSIGN(FalseLocalObject);
};

}  // namespace python
}  // namespace floating_temple

#endif  // PYTHON_FALSE_LOCAL_OBJECT_H_
