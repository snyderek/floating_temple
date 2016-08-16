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

#ifndef ENGINE_EVENT_MATCHING_H_
#define ENGINE_EVENT_MATCHING_H_

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "include/c++/value.h"

namespace floating_temple {
namespace engine {

class ObjectReferenceImpl;
class SharedObject;

bool MethodCallMatches(
    SharedObject* expected_shared_object,
    const std::string& expected_method_name,
    const std::vector<Value>& expected_parameters,
    ObjectReferenceImpl* object_reference,
    const std::string& method_name,
    const std::vector<Value>& parameters,
    const std::unordered_set<SharedObject*>& new_shared_objects,
    std::unordered_map<SharedObject*, ObjectReferenceImpl*>*
        new_object_references,
    std::unordered_set<ObjectReferenceImpl*>* unbound_object_references);

bool ValueMatches(
    const Value& committed_value,
    const Value& pending_value,
    const std::unordered_set<SharedObject*>& new_shared_objects,
    std::unordered_map<SharedObject*, ObjectReferenceImpl*>*
        new_object_references,
    std::unordered_set<ObjectReferenceImpl*>* unbound_object_references);

bool ObjectMatches(
    SharedObject* shared_object,
    ObjectReferenceImpl* object_reference,
    const std::unordered_set<SharedObject*>& new_shared_objects,
    std::unordered_map<SharedObject*, ObjectReferenceImpl*>*
        new_object_references,
    std::unordered_set<ObjectReferenceImpl*>* unbound_object_references);

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_EVENT_MATCHING_H_
