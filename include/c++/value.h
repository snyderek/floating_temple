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

#ifndef INCLUDE_CPP_VALUE_H_
#define INCLUDE_CPP_VALUE_H_

#include <string>

#include "base/integral_types.h"
#include "base/logging.h"

namespace floating_temple {

class ObjectReference;

// A Value object stores a value of one of the primitive types supported by the
// distributed interpreter.
//
// There's also a protocol message analog of this class: ValueProto, defined in
// proto/value_proto.proto. ValueProto is used to transmit values between peers.
// The primary difference between the two classes is that Value represents an
// object as an ObjectReference pointer, whereas ValueProto represents an object
// as an object ID.
class Value {
 public:
  // When an instance of this class is created, its type is initially set to
  // UNINITIALIZED. However, this is not considered a valid value type. You must
  // explicitly set the type by calling one of the setter methods below.
  enum Type { UNINITIALIZED, EMPTY, DOUBLE, FLOAT, INT64, UINT64, BOOL, STRING,
              BYTES, OBJECT_REFERENCE };

  Value();
  Value(const Value& other);
  ~Value();

  int local_type() const { return local_type_; }

  // Returns the type of value stored in this object.
  // TODO(dss): Crash if type_ == UNINITIALIZED.
  Type type() const { return type_; }

  // These getter methods return the value stored in the object, depending on
  // the type of value. You must call the type() method first to determine which
  // getter method to call. Calling the wrong method will cause a crash. Note
  // that the EMPTY type does not have an associated value.
  double double_value() const;
  float float_value() const;
  int64 int64_value() const;
  uint64 uint64_value() const;
  bool bool_value() const;
  const std::string& string_value() const;
  const std::string& bytes_value() const;
  ObjectReference* object_reference() const;

  // These setter methods change the value type and set the associated value (if
  // applicable).
  void set_empty(int local_type);
  void set_double_value(int local_type, double value);
  void set_float_value(int local_type, float value);
  void set_int64_value(int local_type, int64 value);
  void set_uint64_value(int local_type, uint64 value);
  void set_bool_value(int local_type, bool value);
  void set_string_value(int local_type, const std::string& value);
  void set_bytes_value(int local_type, const std::string& value);
  // object_reference must not be NULL.
  void set_object_reference(int local_type, ObjectReference* object_reference);

  Value& operator=(const Value& other);

  std::string Dump() const;

 private:
  void ChangeType(int local_type, Type new_type);

  int local_type_;
  Type type_;

  union {
    double double_value_;
    float float_value_;
    int64 int64_value_;
    uint64 uint64_value_;
    bool bool_value_;
    std::string* string_or_bytes_value_;  // Owned by this object.
    ObjectReference* object_reference_;  // Not owned by this object.
  };
};

inline Value::Value()
    : local_type_(-1),
      type_(UNINITIALIZED) {
}

inline Value::~Value() {
  if (type_ == STRING || type_ == BYTES) {
    delete string_or_bytes_value_;
  }
}

inline double Value::double_value() const {
  CHECK_EQ(type_, DOUBLE);
  return double_value_;
}

inline float Value::float_value() const {
  CHECK_EQ(type_, FLOAT);
  return float_value_;
}

inline int64 Value::int64_value() const {
  CHECK_EQ(type_, INT64);
  return int64_value_;
}

inline uint64 Value::uint64_value() const {
  CHECK_EQ(type_, UINT64);
  return uint64_value_;
}

inline bool Value::bool_value() const {
  CHECK_EQ(type_, BOOL);
  return bool_value_;
}

inline const std::string& Value::string_value() const {
  CHECK_EQ(type_, STRING);
  return *string_or_bytes_value_;
}

inline const std::string& Value::bytes_value() const {
  CHECK_EQ(type_, BYTES);
  return *string_or_bytes_value_;
}

inline ObjectReference* Value::object_reference() const {
  CHECK_EQ(type_, OBJECT_REFERENCE);
  return object_reference_;
}

inline void Value::set_empty(int local_type) {
  ChangeType(local_type, EMPTY);
}

inline void Value::set_double_value(int local_type, double value) {
  ChangeType(local_type, DOUBLE);
  double_value_ = value;
}

inline void Value::set_float_value(int local_type, float value) {
  ChangeType(local_type, FLOAT);
  float_value_ = value;
}

inline void Value::set_int64_value(int local_type, int64 value) {
  ChangeType(local_type, INT64);
  int64_value_ = value;
}

inline void Value::set_uint64_value(int local_type, uint64 value) {
  ChangeType(local_type, UINT64);
  uint64_value_ = value;
}

inline void Value::set_bool_value(int local_type, bool value) {
  ChangeType(local_type, BOOL);
  bool_value_ = value;
}

inline void Value::set_string_value(int local_type, const std::string& value) {
  ChangeType(local_type, STRING);
  *string_or_bytes_value_ = value;
}

inline void Value::set_bytes_value(int local_type, const std::string& value) {
  ChangeType(local_type, BYTES);
  *string_or_bytes_value_ = value;
}

inline void Value::set_object_reference(int local_type,
                                        ObjectReference* object_reference) {
  CHECK(object_reference != nullptr);

  ChangeType(local_type, OBJECT_REFERENCE);
  object_reference_ = object_reference;
}

}  // namespace floating_temple

#endif  // INCLUDE_CPP_VALUE_H_
