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

#ifndef INCLUDE_CPP_THREAD_H_
#define INCLUDE_CPP_THREAD_H_

#include <string>
#include <vector>

#include "include/c++/value.h"

namespace floating_temple {

class LocalObject;
class ObjectReference;

// This interface is implemented by the peer. The local interpreter uses it to
// perform any operations that require assistance from the peer during the
// duration of a method call. The local interpreter should not store a pointer
// to the Thread instance beyond the duration of the method call.
//
// TODO(dss): Rename this class to MethodContext.
class Thread {
 public:
  virtual ~Thread() {}

  // Begins a transaction in this thread. Transactions may be nested by calling
  // BeginTransaction more than once without an intervening call to
  // EndTransaction. Method calls executed within a transaction will not be
  // propagated to remote peers until the outermost transaction is committed.
  // ("Method calls" in this context refers to methods in the interpreted
  // language, not C++ methods.)
  //
  // Returns true if the operation was successful. Returns false if a conflict
  // occurred with another peer.
  //
  // IMPORTANT: If BeginTransaction returns false, the caller must immediately
  // return from LocalObject::InvokeMethod.
  virtual bool BeginTransaction() = 0;

  // Ends the pending transaction that was begun most recently. If that
  // transaction is the outermost transaction, the transaction will be
  // committed and the method calls that were executed within the transaction
  // will be propagated to remote peers.
  //
  // Returns true if the operation was successful. Returns false if a conflict
  // occurred with another peer.
  //
  // IMPORTANT: If EndTransaction returns false, the caller must immediately
  // return from LocalObject::InvokeMethod.
  virtual bool EndTransaction() = 0;

  // Returns a reference to a newly created shared object that corresponds to an
  // existing local object. *initial_version is the initial version of the local
  // object; it may later be cloned via LocalObject::Clone to create
  // additional versions of the object.
  //
  // The peer takes ownership of *initial_version. The caller must not take
  // ownership of the returned ObjectReference instance.
  //
  // If 'name' is not the empty string, it will be used as the name for the new
  // object. Object names are global: if a remote peer creates an object with
  // the same name as an object on the local peer, the two objects will be
  // treated as a single object by the distributed interpreter.
  //
  // TODO(dss): The local interpreter should take ownership of the
  // ObjectReference instance. Otherwise, the peer has no way of knowing when
  // the local interpreter is done using it.
  virtual ObjectReference* CreateObject(LocalObject* initial_version,
                                        const std::string& name) = 0;

  // Calls the specified method on the specified object, and copies the return
  // value to *return_value. Depending on how the interpreted code is being
  // executed, CallMethod may return a canned value instead of actually calling
  // the method. However, the local interpreter should not be concerned about
  // the details of this subterfuge.
  //
  // Returns true if the method call was successful (possibly as a mock method
  // call). Returns false if a conflict occurred with another peer.
  //
  // IMPORTANT: If CallMethod returns false, the caller must immediately return
  // from LocalObject::InvokeMethod.
  virtual bool CallMethod(ObjectReference* object_reference,
                          const std::string& method_name,
                          const std::vector<Value>& parameters,
                          Value* return_value) = 0;

  virtual bool ObjectsAreIdentical(const ObjectReference* a,
                                   const ObjectReference* b) const = 0;
};

}  // namespace floating_temple

#endif  // INCLUDE_CPP_THREAD_H_
