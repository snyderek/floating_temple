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

#ifndef BASE_SCOPED_PTR_H_
#define BASE_SCOPED_PTR_H_

#include <algorithm>
#include <cstddef>

#include "base/logging.h"
#include "base/macros.h"

namespace floating_temple {

// Convenience class that deletes an object when the scoped_ptr instance goes
// out of scope.
template<typename T> class scoped_ptr {
 public:
  typedef T element_type;

  explicit scoped_ptr(T* ptr = NULL);
  ~scoped_ptr();

  // ptr may be NULL.
  void reset(T* ptr);

  // These methods crash if the pointer is NULL.
  T& operator*() const;
  T* operator->() const;

  T* get() const;

  // Relinquishes ownership of the pointed-to object and returns the pointer.
  T* release();

  void swap(scoped_ptr& other);

 private:
  T* GetPointer() const;

  T* ptr_;

  DISALLOW_COPY_AND_ASSIGN(scoped_ptr);
};

template<typename T> scoped_ptr<T>::scoped_ptr(T* ptr)
    : ptr_(ptr) {
}

template<typename T> scoped_ptr<T>::~scoped_ptr() {
  delete ptr_;
}

template<typename T> void scoped_ptr<T>::reset(T* ptr) {
  if (ptr != ptr_) {
    delete ptr_;
    ptr_ = ptr;
  }
}

template<typename T> inline T* scoped_ptr<T>::GetPointer() const {
  CHECK(ptr_ != NULL);
  return ptr_;
}

template<typename T> inline T& scoped_ptr<T>::operator*() const {
  return *GetPointer();
}

template<typename T> inline T* scoped_ptr<T>::operator->() const {
  return GetPointer();
}

template<typename T> inline T* scoped_ptr<T>::get() const {
  return ptr_;
}

template<typename T> T* scoped_ptr<T>::release() {
  T* const ptr = ptr_;
  ptr_ = NULL;
  return ptr;
}

template<typename T> void scoped_ptr<T>::swap(scoped_ptr& other) {
  std::swap(ptr_, other.ptr_);
}

}  // namespace floating_temple

#endif  // BASE_SCOPED_PTR_H_
