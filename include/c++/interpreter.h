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

#ifndef INCLUDE_CPP_INTERPRETER_H_
#define INCLUDE_CPP_INTERPRETER_H_

#include <cstddef>

namespace floating_temple {

class DeserializationContext;
class LocalObject;

// This interface is implemented by the local interpreter. It represents the
// local interpreter itself.
//
// The (human) implementer of a local interpreter is responsible for designing
// the serialization protocol for that interpreter's objects. From the point of
// view of the Floating Temple engine, a serialized object is just an opaque
// sequence of bytes of known length. The engine doesn't even know the type of a
// serialized object, and so the local interpreter is responsible for encoding
// that information within the serialization protocol.
//
// Derived classes must be thread-safe.
//
// TODO(dss): Consider relaxing the thread-safety requirement for this class.
class Interpreter {
 public:
  virtual ~Interpreter() {}

  // Deserializes an object and creates it in the local interpreter.
  //
  // 'buffer' points to a buffer that contains the serialized form of the local
  // object.
  //
  // 'buffer_size' is the size of the buffer in bytes.
  //
  // 'context' points to a DeserializationContext instance that can be used by
  // the local interpreter to convert object indexes to object references. This
  // instance is valid only for the duration of the call to DeserializeObject.
  // The local interpreter must not take ownership of the DeserializationContext
  // instance.
  //
  // Returns a pointer to a newly created LocalObject instance. The
  // caller will take ownership of this instance.
  virtual LocalObject* DeserializeObject(
      const void* buffer, std::size_t buffer_size,
      DeserializationContext* context) = 0;
};

}  // namespace floating_temple

#endif  // INCLUDE_CPP_INTERPRETER_H_
