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

#ifndef INCLUDE_CPP_DESERIALIZATION_CONTEXT_H_
#define INCLUDE_CPP_DESERIALIZATION_CONTEXT_H_

namespace floating_temple {

class ObjectReference;

// This interface is implemented by the peer. It can be used by the local
// interpreter to convert object indexes to object references during
// deserialization of a local object.
//
// This class is not thread-safe. It's intended to be used only by the thread
// that called Interpreter::DeserializeObject.
class DeserializationContext {
 public:
  virtual ~DeserializationContext() {}

  // Returns the object reference that corresponds to the given object index.
  // The index must have been created by a previous call to
  // SerializationContext::GetIndexForObjectReference, possibly on a different
  // machine. This method may be called repeatedly with the same index, and will
  // always return the same ObjectReference pointer.
  virtual ObjectReference* GetObjectReferenceByIndex(int index) = 0;
};

}  // namespace floating_temple

#endif  // INCLUDE_CPP_DESERIALIZATION_CONTEXT_H_
