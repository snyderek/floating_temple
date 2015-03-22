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

#ifndef BASE_LINKED_PTR_H_
#define BASE_LINKED_PTR_H_

#include "base/logging.h"

namespace floating_temple {

// A shared pointer implementation. When an instance of this class is copied,
// all of the copies (including the original instance) form a circular linked
// list with each other. When the last copy is destroyed, the pointed-to object
// is deleted.
//
// Memory usage is efficient: a linked_ptr instance is the size of two regular
// pointers, and no additional memory is allocated (except, of course, for the
// object being pointed to). The destructor takes O(N) time, where N is the
// number of linked_ptr instances in the group. Because of the relatively large
// time complexity, linked_ptr is intended to be used for the elements in an STL
// container, where a copy constructor is required but the number of copies is
// expected to be small.
template<typename T> class linked_ptr {
 public:
  typedef T element_type;

  explicit linked_ptr(T* ptr = nullptr);
  linked_ptr(const linked_ptr<T>& other);
  ~linked_ptr();

  // ptr may be NULL.
  void reset(T* ptr);

  // These methods crash if the pointer is NULL.
  T& operator*() const;
  T* operator->() const;

  T* get() const;

  // Relinquishes ownership of the pointed-to object and returns the pointer.
  // Crashes if this is not the only linked_ptr instance pointing to the object.
  T* release();

  linked_ptr<T>& operator=(const linked_ptr<T>& other);

 private:
  void RemoveFromList();

  T* ptr_;
  mutable linked_ptr<T>* next_;
};

template<typename T> linked_ptr<T>::linked_ptr(T* ptr)
    : ptr_(ptr),
      next_(this) {
}

template<typename T> linked_ptr<T>::linked_ptr(const linked_ptr<T>& other)
    : ptr_(other.ptr_),
      next_(other.next_) {
  other.next_ = this;
}

template<typename T> linked_ptr<T>::~linked_ptr() {
  RemoveFromList();
}

template<typename T> void linked_ptr<T>::reset(T* ptr) {
  if (ptr != ptr_) {
    RemoveFromList();
    ptr_ = ptr;
    next_ = this;
  }
}

template<typename T> inline T& linked_ptr<T>::operator*() const {
  CHECK(ptr_ != nullptr);
  return *ptr_;
}

template<typename T> inline T* linked_ptr<T>::operator->() const {
  CHECK(ptr_ != nullptr);
  return ptr_;
}

template<typename T> inline T* linked_ptr<T>::get() const {
  return ptr_;
}

template<typename T> T* linked_ptr<T>::release() {
  CHECK_EQ(next_, this);

  T* const ptr = ptr_;
  ptr_ = nullptr;
  return ptr;
}

template<typename T> linked_ptr<T>& linked_ptr<T>::operator=(
    const linked_ptr<T>& other) {
  ptr_ = other.ptr_;
  next_ = other.next_;
  other.next_ = this;

  return *this;
}

template<typename T> void linked_ptr<T>::RemoveFromList() {
  if (next_ == this) {
    delete ptr_;
  } else {
    linked_ptr<T>* node = next_;
    while (node->next_ != this) {
      node = node->next_;
    }
    node->next_ = next_;
  }
}

template<typename T> inline linked_ptr<T> make_linked_ptr(T* ptr) {
  return linked_ptr<T>(ptr);
}

}  // namespace floating_temple

#endif  // BASE_LINKED_PTR_H_
