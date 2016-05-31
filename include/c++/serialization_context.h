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

#ifndef INCLUDE_CPP_SERIALIZATION_CONTEXT_H_
#define INCLUDE_CPP_SERIALIZATION_CONTEXT_H_

namespace floating_temple {

class ObjectReference;

// This interface is implemented by the peer. It can be used by the local
// interpreter to convert object references to object indexes during
// serialization of a local object. Object indexes are useful because they can
// be included in the serialized form of a local object. Object references, on
// the other hand, are only valid within the local process.
//
// This class is not thread-safe. It's intended to be used only by the thread
// that called LocalObject::Serialize.
class SerializationContext {
 public:
  virtual ~SerializationContext() {}

  // Returns the object index that corresponds to the given object reference.
  // This method may be called repeatedly with the same ObjectReference pointer,
  // and will always return the same object index.
  //
  // 'object_reference' must not be NULL.
  virtual int GetIndexForObjectReference(ObjectReference* object_reference) = 0;
};

}  // namespace floating_temple

#endif  // INCLUDE_CPP_SERIALIZATION_CONTEXT_H_
