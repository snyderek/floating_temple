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

#ifndef ENGINE_SERIALIZE_LOCAL_OBJECT_TO_STRING_H_
#define ENGINE_SERIALIZE_LOCAL_OBJECT_TO_STRING_H_

#include <string>
#include <vector>

namespace floating_temple {

class Interpreter;
class LocalObject;

namespace engine {

class ObjectReferenceImpl;

void SerializeLocalObjectToString(
    const LocalObject* local_object, std::string* data,
    std::vector<ObjectReferenceImpl*>* object_references);

LocalObject* DeserializeLocalObjectFromString(
    Interpreter* interpreter, const std::string& data,
    const std::vector<ObjectReferenceImpl*>& object_references);

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_SERIALIZE_LOCAL_OBJECT_TO_STRING_H_
