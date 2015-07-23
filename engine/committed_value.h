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

#ifndef ENGINE_COMMITTED_VALUE_H_
#define ENGINE_COMMITTED_VALUE_H_

#include <string>

#include "base/integral_types.h"
#include "base/logging.h"

namespace floating_temple {
namespace engine {

class SharedObject;

class CommittedValue {
 public:
  enum Type { UNINITIALIZED, EMPTY, DOUBLE, FLOAT, INT64, UINT64, BOOL, STRING,
              BYTES, SHARED_OBJECT };

  CommittedValue();
  CommittedValue(const CommittedValue& other);
  ~CommittedValue();

  int local_type() const { return local_type_; }
  Type type() const { return type_; }

  double double_value() const;
  float float_value() const;
  int64 int64_value() const;
  uint64 uint64_value() const;
  bool bool_value() const;
  const std::string& string_value() const;
  const std::string& bytes_value() const;
  SharedObject* shared_object() const;

  void set_local_type(int local_type) { local_type_ = local_type; }

  void set_empty();
  void set_double_value(double value);
  void set_float_value(float value);
  void set_int64_value(int64 value);
  void set_uint64_value(uint64 value);
  void set_bool_value(bool value);
  void set_string_value(const std::string& value);
  void set_bytes_value(const std::string& value);
  // shared_object must not be NULL.
  void set_shared_object(SharedObject* shared_object);

  CommittedValue& operator=(const CommittedValue& other);

  std::string Dump() const;

 private:
  void ChangeType(Type new_type);

  int local_type_;
  Type type_;

  union {
    double double_value_;
    float float_value_;
    int64 int64_value_;
    uint64 uint64_value_;
    bool bool_value_;
    std::string* string_or_bytes_value_;  // Owned by this object.
    SharedObject* shared_object_;  // Not owned by this object.
  };
};

inline CommittedValue::CommittedValue()
    : local_type_(-1),
      type_(UNINITIALIZED) {
}

inline CommittedValue::~CommittedValue() {
  if (type_ == STRING || type_ == BYTES) {
    delete string_or_bytes_value_;
  }
}

inline double CommittedValue::double_value() const {
  CHECK_EQ(type_, DOUBLE);
  return double_value_;
}

inline float CommittedValue::float_value() const {
  CHECK_EQ(type_, FLOAT);
  return float_value_;
}

inline int64 CommittedValue::int64_value() const {
  CHECK_EQ(type_, INT64);
  return int64_value_;
}

inline uint64 CommittedValue::uint64_value() const {
  CHECK_EQ(type_, UINT64);
  return uint64_value_;
}

inline bool CommittedValue::bool_value() const {
  CHECK_EQ(type_, BOOL);
  return bool_value_;
}

inline const std::string& CommittedValue::string_value() const {
  CHECK_EQ(type_, STRING);
  return *string_or_bytes_value_;
}

inline const std::string& CommittedValue::bytes_value() const {
  CHECK_EQ(type_, BYTES);
  return *string_or_bytes_value_;
}

inline SharedObject* CommittedValue::shared_object() const {
  CHECK_EQ(type_, SHARED_OBJECT);
  return shared_object_;
}

inline void CommittedValue::set_empty() {
  ChangeType(EMPTY);
}

inline void CommittedValue::set_double_value(double value) {
  ChangeType(DOUBLE);
  double_value_ = value;
}

inline void CommittedValue::set_float_value(float value) {
  ChangeType(FLOAT);
  float_value_ = value;
}

inline void CommittedValue::set_int64_value(int64 value) {
  ChangeType(INT64);
  int64_value_ = value;
}

inline void CommittedValue::set_uint64_value(uint64 value) {
  ChangeType(UINT64);
  uint64_value_ = value;
}

inline void CommittedValue::set_bool_value(bool value) {
  ChangeType(BOOL);
  bool_value_ = value;
}

inline void CommittedValue::set_string_value(const std::string& value) {
  ChangeType(STRING);
  *string_or_bytes_value_ = value;
}

inline void CommittedValue::set_bytes_value(const std::string& value) {
  ChangeType(BYTES);
  *string_or_bytes_value_ = value;
}

inline void CommittedValue::set_shared_object(SharedObject* shared_object) {
  CHECK(shared_object != nullptr);

  ChangeType(SHARED_OBJECT);
  shared_object_ = shared_object;
}

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_COMMITTED_VALUE_H_
