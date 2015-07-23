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

#include "engine/serialize_local_object_to_string.h"

#include <cstddef>
#include <string>
#include <vector>

#include "base/logging.h"
#include "engine/deserialization_context_impl.h"
#include "engine/serialization_context_impl.h"
#include "include/c++/interpreter.h"
#include "include/c++/versioned_local_object.h"

using std::size_t;
using std::string;
using std::vector;

namespace floating_temple {
namespace engine {
namespace {

size_t TryToSerialize(const VersionedLocalObject* local_object,
                      void* buffer,
                      size_t buffer_size,
                      vector<ObjectReferenceImpl*>* object_references) {
  CHECK(local_object != nullptr);
  CHECK(object_references != nullptr);

  object_references->clear();

  SerializationContextImpl context(object_references);
  return local_object->Serialize(buffer, buffer_size, &context);
}

}  // namespace

void SerializeLocalObjectToString(
    const VersionedLocalObject* local_object, string* data,
    vector<ObjectReferenceImpl*>* object_references) {
  CHECK(data != nullptr);

  char static_buffer[1000];
  const size_t data_size = TryToSerialize(local_object, static_buffer,
                                          sizeof static_buffer,
                                          object_references);

  if (data_size <= sizeof static_buffer) {
    data->assign(static_buffer, data_size);
    return;
  }

  char* const dynamic_buffer = new char[data_size];
  CHECK_EQ(TryToSerialize(local_object, dynamic_buffer, data_size,
                          object_references), data_size);

  data->assign(dynamic_buffer, data_size);
  delete[] dynamic_buffer;
}

VersionedLocalObject* DeserializeLocalObjectFromString(
    Interpreter* interpreter, const string& data,
    const vector<ObjectReferenceImpl*>& object_references) {
  CHECK(interpreter != nullptr);

  DeserializationContextImpl context(&object_references);
  return interpreter->DeserializeObject(data.data(),
                                        static_cast<size_t>(data.size()),
                                        &context);
}

}  // namespace engine
}  // namespace floating_temple
