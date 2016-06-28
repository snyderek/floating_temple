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

#ifndef ENGINE_RECORDING_METHOD_CONTEXT_H_
#define ENGINE_RECORDING_METHOD_CONTEXT_H_

#include <memory>

#include "base/macros.h"
#include "include/c++/method_context.h"

namespace floating_temple {
namespace engine {

class LiveObject;
class ObjectReferenceImpl;
class RecordingThreadInternalInterface;

// TODO(dss): Rename this class. It's no longer used by just the RecordingThread
// class.
class RecordingMethodContext : public MethodContext {
 public:
  RecordingMethodContext(
      RecordingThreadInternalInterface* recording_thread,
      ObjectReferenceImpl* current_object_reference,
      const std::shared_ptr<LiveObject>& current_live_object);
  ~RecordingMethodContext() override;

  void set_recording_thread(RecordingThreadInternalInterface* recording_thread);

  bool BeginTransaction() override;
  bool EndTransaction() override;
  ObjectReference* CreateObject(LocalObject* initial_version,
                                const std::string& name) override;
  bool CallMethod(ObjectReference* object_reference,
                  const std::string& method_name,
                  const std::vector<Value>& parameters,
                  Value* return_value) override;
  bool ObjectsAreIdentical(const ObjectReference* a,
                           const ObjectReference* b) const override;

 private:
  RecordingThreadInternalInterface* recording_thread_;
  ObjectReferenceImpl* const current_object_reference_;
  const std::shared_ptr<LiveObject> current_live_object_;

  DISALLOW_COPY_AND_ASSIGN(RecordingMethodContext);
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_RECORDING_METHOD_CONTEXT_H_
