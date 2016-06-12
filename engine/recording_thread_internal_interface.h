// Floating Temple
// Copyright 2016 Derek S. Snyder
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

#ifndef ENGINE_RECORDING_THREAD_INTERNAL_INTERFACE_H_
#define ENGINE_RECORDING_THREAD_INTERNAL_INTERFACE_H_

#include <memory>
#include <string>
#include <vector>

#include "include/c++/value.h"

namespace floating_temple {

class LocalObject;

namespace engine {

class LiveObject;
class ObjectReferenceImpl;
class TransactionId;

class RecordingThreadInternalInterface {
 public:
  virtual ~RecordingThreadInternalInterface() {}

  virtual bool BeginTransaction(
      ObjectReferenceImpl* caller_object_reference,
      const std::shared_ptr<LiveObject>& caller_live_object) = 0;
  virtual bool EndTransaction(
      ObjectReferenceImpl* caller_object_reference,
      const std::shared_ptr<LiveObject>& caller_live_object) = 0;
  virtual ObjectReferenceImpl* CreateObject(LocalObject* initial_version,
                                            const std::string& name) = 0;
  virtual bool CallMethod(const TransactionId& base_transaction_id,
                          ObjectReferenceImpl* caller_object_reference,
                          const std::shared_ptr<LiveObject>& caller_live_object,
                          ObjectReferenceImpl* callee_object_reference,
                          const std::string& method_name,
                          const std::vector<Value>& parameters,
                          Value* return_value) = 0;
  virtual bool ObjectsAreIdentical(const ObjectReferenceImpl* a,
                                   const ObjectReferenceImpl* b) const = 0;
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_RECORDING_THREAD_INTERNAL_INTERFACE_H_
