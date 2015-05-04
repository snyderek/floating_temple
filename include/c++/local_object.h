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

#ifndef INCLUDE_CPP_LOCAL_OBJECT_H_
#define INCLUDE_CPP_LOCAL_OBJECT_H_

#include <cstddef>
#include <string>
#include <vector>

#include "include/c++/value.h"

namespace floating_temple {

class PeerObject;
class SerializationContext;
class Thread;

// TODO(dss): Rewrite the following paragraph. It's confusing.
//
// This interface is implemented by the local interpreter. It represents one
// version of a particular object in the local interpreter. It's possible for
// multiple LocalObject objects to refer to the same local interpreter object.
//
// Subclasses of this class are thread-safe.
//
// TODO(dss): Create a separate interface class for local objects that are
// unversioned. The unversioned interface won't need the Clone and Serialize
// methods.
class LocalObject {
 public:
  virtual ~LocalObject() {}

  // Returns a pointer to a new LocalObject object that is a clone of *this. The
  // caller must take ownership of the returned LocalObject object.
  virtual LocalObject* Clone() const = 0;

  // buffer points to a writable buffer, and buffer_size is the maximum number
  // of bytes that can be written to the buffer.
  //
  // If the buffer is large enough, this method serializes *this to the buffer
  // and returns the number of bytes written. Otherwise, it leaves the buffer
  // untouched and returns the minimum required buffer size, in bytes.
  virtual std::size_t Serialize(void* buffer, std::size_t buffer_size,
                                SerializationContext* context) const = 0;

  // Invokes the specified method on *this and passes it the specified
  // parameters. method_name must not be empty. The method must exist on the
  // object, and the number and types of the parameters must be correct.
  //
  // If the method execution is successful, the return value from the method
  // will be placed in *return_value. If the method execution is not successful
  // (because a call to Thread::CallMethod returned false), *return_value will
  // be left unchanged.
  //
  // This object does not store the Thread pointer.
  //
  // TODO(dss): Support exceptions.
  virtual void InvokeMethod(Thread* thread,
                            PeerObject* peer_object,
                            const std::string& method_name,
                            const std::vector<Value>& parameters,
                            Value* return_value) = 0;

  virtual std::string Dump() const = 0;
};

}  // namespace floating_temple

#endif  // INCLUDE_CPP_LOCAL_OBJECT_H_
