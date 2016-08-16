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

#include "engine/event_matching.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/escape.h"
#include "base/logging.h"
#include "engine/object_reference_impl.h"
#include "engine/shared_object.h"
#include "include/c++/value.h"

using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

namespace floating_temple {
namespace engine {

bool MethodCallMatches(
    SharedObject* expected_shared_object,
    const string& expected_method_name,
    const vector<Value>& expected_parameters,
    ObjectReferenceImpl* object_reference,
    const string& method_name,
    const vector<Value>& parameters,
    const unordered_set<SharedObject*>& new_shared_objects,
    unordered_map<SharedObject*, ObjectReferenceImpl*>* new_object_references,
    unordered_set<ObjectReferenceImpl*>* unbound_object_references) {
  CHECK(object_reference != nullptr);

  if (!ObjectMatches(expected_shared_object, object_reference,
                     new_shared_objects, new_object_references,
                     unbound_object_references)) {
    VLOG(2) << "Objects don't match.";
    return false;
  }

  if (expected_method_name != method_name) {
    VLOG(2) << "Method names don't match (\"" << CEscape(expected_method_name)
            << "\" != \"" << CEscape(method_name) << "\").";
    return false;
  }

  if (expected_parameters.size() != parameters.size()) {
    VLOG(2) << "Parameter counts don't match (" << expected_parameters.size()
            << " != " << parameters.size() << ").";
    return false;
  }

  for (vector<Value>::size_type i = 0; i < expected_parameters.size(); ++i) {
    const Value& expected_parameter = expected_parameters[i];

    if (!ValueMatches(expected_parameter, parameters[i], new_shared_objects,
                      new_object_references, unbound_object_references)) {
      VLOG(2) << "Parameter " << i << ": values don't match.";
      return false;
    }
  }

  return true;
}

#define COMPARE_FIELDS(enum_const, getter_method) \
  case Value::enum_const: \
    return committed_value.getter_method() == pending_value.getter_method();

bool ValueMatches(
    const Value& committed_value,
    const Value& pending_value,
    const unordered_set<SharedObject*>& new_shared_objects,
    unordered_map<SharedObject*, ObjectReferenceImpl*>* new_object_references,
    unordered_set<ObjectReferenceImpl*>* unbound_object_references) {
  if (committed_value.local_type() != pending_value.local_type()) {
    return false;
  }

  const Value::Type committed_value_type = committed_value.type();
  const Value::Type pending_value_type = pending_value.type();

  if (committed_value_type != pending_value_type) {
    return false;
  }

  switch (committed_value_type) {
    case Value::EMPTY:
      return true;

    COMPARE_FIELDS(DOUBLE, double_value);
    COMPARE_FIELDS(FLOAT, float_value);
    COMPARE_FIELDS(INT64, int64_value);
    COMPARE_FIELDS(UINT64, uint64_value);
    COMPARE_FIELDS(BOOL, bool_value);
    COMPARE_FIELDS(STRING, string_value);
    COMPARE_FIELDS(BYTES, bytes_value);

    case Value::OBJECT_REFERENCE:
      return ObjectMatches(
          static_cast<ObjectReferenceImpl*>(committed_value.object_reference())
              ->shared_object(),
          static_cast<ObjectReferenceImpl*>(pending_value.object_reference()),
          new_shared_objects, new_object_references, unbound_object_references);

    default:
      LOG(FATAL) << "Unexpected committed value type: "
                 << static_cast<int>(committed_value_type);
  }
}

#undef COMPARE_FIELDS

bool ObjectMatches(
    SharedObject* shared_object,
    ObjectReferenceImpl* object_reference,
    const unordered_set<SharedObject*>& new_shared_objects,
    unordered_map<SharedObject*, ObjectReferenceImpl*>* new_object_references,
    unordered_set<ObjectReferenceImpl*>* unbound_object_references) {
  CHECK(shared_object != nullptr);
  CHECK(object_reference != nullptr);
  CHECK(new_object_references != nullptr);
  CHECK(unbound_object_references != nullptr);

  const bool shared_object_is_new =
      (new_shared_objects.find(shared_object) != new_shared_objects.end());

  const unordered_set<ObjectReferenceImpl*>::iterator unbound_it =
      unbound_object_references->find(object_reference);
  const bool object_reference_is_unbound =
      (unbound_it != unbound_object_references->end());

  if (shared_object_is_new && object_reference_is_unbound) {
    const auto insert_result = new_object_references->emplace(shared_object,
                                                              nullptr);

    if (!insert_result.second) {
      return false;
    }

    insert_result.first->second = object_reference;
    unbound_object_references->erase(unbound_it);

    return true;
  }

  const unordered_map<SharedObject*, ObjectReferenceImpl*>::const_iterator
      new_object_reference_it = new_object_references->find(shared_object);

  if (new_object_reference_it != new_object_references->end() &&
      new_object_reference_it->second == object_reference) {
    return true;
  }

  return shared_object->HasObjectReference(object_reference);
}

}  // namespace engine
}  // namespace floating_temple
