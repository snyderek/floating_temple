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

#ifndef PYTHON_PROGRAM_OBJECT_H_
#define PYTHON_PROGRAM_OBJECT_H_

#include "third_party/Python-3.4.2/Include/Python.h"

#include <cstdio>
#include <string>

#include "base/macros.h"
#include "include/c++/versioned_local_object.h"

namespace floating_temple {
namespace python {

class ProgramObject : public VersionedLocalObject {
 public:
  ProgramObject(std::FILE* fp, const std::string& source_file_name,
                PyObject* globals);

  VersionedLocalObject* Clone() const override;
  std::size_t Serialize(void* buffer, std::size_t buffer_size,
                        SerializationContext* context) const override;
  void InvokeMethod(Thread* thread,
                    PeerObject* peer_object,
                    const std::string& method_name,
                    const std::vector<Value>& parameters,
                    Value* return_value) override;
  std::string Dump() const override;

 private:
  std::FILE* const fp_;
  const std::string source_file_name_;
  PyObject* const globals_;

  DISALLOW_COPY_AND_ASSIGN(ProgramObject);
};

}  // namespace python
}  // namespace floating_temple

#endif  // PYTHON_PROGRAM_OBJECT_H_
