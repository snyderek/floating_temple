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

#ifndef BASE_INTRUSIVE_PTR_H_
#define BASE_INTRUSIVE_PTR_H_

#include "base/logging.h"

namespace floating_temple {

// A shared pointer implementation. The pointed-to object is deleted when the
// last copy of the intrusive pointer is destroyed.
//
// The pointed-to object is responsible for keeping track of the reference
// count. The reference count must initially be 1.
//
// class T must define the following methods:
//
//   void IncrementRefCount();
//
//   // Returns true if the reference count is 0.
//   bool DecrementRefCount();
//
template<class T> class intrusive_ptr {
 public:
  typedef T element_type;

  explicit intrusive_ptr(T* ptr = nullptr);
  intrusive_ptr(const intrusive_ptr<T>& other);
  ~intrusive_ptr();

  // ptr may be NULL.
  void reset(T* ptr);

  intrusive_ptr<T>& operator=(const intrusive_ptr<T>& other);

  T& operator*() const;
  T* operator->() const;

  T* get() const;

 private:
  void IncrementRefCount();
  void DecrementRefCount();

  T* ptr_;
};

template<class T> intrusive_ptr<T>::intrusive_ptr(T* ptr)
    : ptr_(ptr) {
}

template<class T> intrusive_ptr<T>::intrusive_ptr(const intrusive_ptr<T>& other)
    : ptr_(other.ptr_) {
  IncrementRefCount();
}

template<class T> intrusive_ptr<T>::~intrusive_ptr() {
  DecrementRefCount();
}

template<class T> void intrusive_ptr<T>::reset(T* ptr) {
  if (ptr_ == ptr) {
    return;
  }

  DecrementRefCount();
  ptr_ = ptr;
}

template<class T> intrusive_ptr<T>& intrusive_ptr<T>::operator=(
    const intrusive_ptr<T>& other) {
  if (ptr_ != other.ptr_) {
    DecrementRefCount();
    ptr_ = other.ptr_;
    IncrementRefCount();
  }

  return *this;
}

template<class T> inline T& intrusive_ptr<T>::operator*() const {
  CHECK(ptr_ != nullptr);
  return *ptr_;
}

template<class T> inline T* intrusive_ptr<T>::operator->() const {
  CHECK(ptr_ != nullptr);
  return ptr_;
}

template<class T> T* intrusive_ptr<T>::get() const {
  return ptr_;
}

template<class T> void intrusive_ptr<T>::IncrementRefCount() {
  if (ptr_ != nullptr) {
    ptr_->IncrementRefCount();
  }
}

template<class T> void intrusive_ptr<T>::DecrementRefCount() {
  if (ptr_ != nullptr) {
    if (ptr_->DecrementRefCount()) {
      delete ptr_;
    }

    ptr_ = nullptr;
  }
}

template<class T> inline intrusive_ptr<T> make_intrusive_ptr(T* ptr) {
  return intrusive_ptr<T>(ptr);
}

}  // namespace floating_temple

#endif  // BASE_INTRUSIVE_PTR_H_
