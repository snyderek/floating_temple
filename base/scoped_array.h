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

#ifndef BASE_SCOPED_ARRAY_H_
#define BASE_SCOPED_ARRAY_H_

#include <algorithm>
#include <cstddef>

#include "base/logging.h"
#include "base/macros.h"

namespace floating_temple {

// Convenience class that deletes an array (using delete[]) when the
// scoped_array instance goes out of scope.
//
// This class is analogous to scoped_ptr; use scoped_array when the memory was
// allocated with new[], rather than new.
template<typename T> class scoped_array {
 public:
  explicit scoped_array(T* array = NULL);
  ~scoped_array();

  // array may be NULL.
  void reset(T* array);

  // Element access. The behavior is undefined if index is not a valid index
  // into the array. Crashes if the array pointer is NULL.
  T& operator[](int index) const;
  // Returns a pointer to the whole array.
  T* get() const;

  // Relinquishes ownership of the pointed-to array and returns the pointer.
  T* release();

  void swap(scoped_array& other);

 private:
  T* array_;

  DISALLOW_COPY_AND_ASSIGN(scoped_array);
};

template<typename T> scoped_array<T>::scoped_array(T* array)
    : array_(array) {
}

template<typename T> scoped_array<T>::~scoped_array() {
  delete[] array_;
}

template<typename T> void scoped_array<T>::reset(T* array) {
  if (array != array_) {
    delete[] array_;
    array_ = array;
  }
}

template<typename T> inline T& scoped_array<T>::operator[](int index) const {
  CHECK(array_ != NULL);
  CHECK_GE(index, 0);
  return array_[index];
}

template<typename T> inline T* scoped_array<T>::get() const {
  return array_;
}

template<typename T> T* scoped_array<T>::release() {
  T* const array = array_;
  array_ = NULL;
  return array;
}

template<typename T> void scoped_array<T>::swap(scoped_array& other) {
  std::swap(array_, other.array_);
}

}  // namespace floating_temple

#endif  // BASE_SCOPED_ARRAY_H_
