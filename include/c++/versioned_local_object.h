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

#ifndef INCLUDE_CPP_VERSIONED_LOCAL_OBJECT_H_
#define INCLUDE_CPP_VERSIONED_LOCAL_OBJECT_H_

#include <cstddef>

#include "include/c++/local_object.h"

namespace floating_temple {

class SerializationContext;

// Derived classes must be thread-safe.
class VersionedLocalObject : public LocalObject {
 public:
  // Returns a pointer to a new VersionedLocalObject object that is a clone of
  // *this. The caller must take ownership of the returned VersionedLocalObject
  // object.
  virtual VersionedLocalObject* Clone() const = 0;

  // buffer points to a writable buffer, and buffer_size is the maximum number
  // of bytes that can be written to the buffer.
  //
  // If the buffer is large enough, this method serializes *this to the buffer
  // and returns the number of bytes written. Otherwise, it leaves the buffer
  // untouched and returns the minimum required buffer size, in bytes.
  virtual std::size_t Serialize(void* buffer, std::size_t buffer_size,
                                SerializationContext* context) const = 0;
};

}  // namespace floating_temple

#endif  // INCLUDE_CPP_VERSIONED_LOCAL_OBJECT_H_
