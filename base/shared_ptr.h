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

#ifndef BASE_SHARED_PTR_H_
#define BASE_SHARED_PTR_H_

#include <cstddef>

#include "base/logging.h"

namespace floating_temple {

template<typename T> class const_shared_ptr;

// A reference-counted shared pointer implementation. The pointed-to object is
// deleted when the last copy of the shared pointer is destroyed.
template<typename T> class shared_ptr {
 public:
  typedef T element_type;

  explicit shared_ptr(T* ptr = NULL);
  shared_ptr(const shared_ptr<T>& other);
  ~shared_ptr();

  // ptr may be NULL.
  void reset(T* ptr);

  shared_ptr<T>& operator=(const shared_ptr<T>& other);

  T& operator*() const;
  T* operator->() const;

  T* get() const;

 private:
  struct Node {
    T* ptr;  // Not NULL
    int ref_count;
  };

  void IncrementRefCount();
  void DecrementRefCount();

  // If the shared pointer is set to NULL, node_ will be NULL.
  Node* node_;

  friend class const_shared_ptr<T>;
};

template<typename T> shared_ptr<T>::shared_ptr(T* ptr) {
  if (ptr == NULL) {
    node_ = NULL;
  } else {
    node_ = new Node();
    node_->ptr = ptr;
    node_->ref_count = 1;
  }
}

template<typename T> shared_ptr<T>::shared_ptr(const shared_ptr<T>& other)
    : node_(other.node_) {
  IncrementRefCount();
}

template<typename T> shared_ptr<T>::~shared_ptr() {
  DecrementRefCount();
}

template<typename T> void shared_ptr<T>::reset(T* ptr) {
  if (node_ != NULL && node_->ptr == ptr) {
    return;
  }

  DecrementRefCount();

  if (ptr != NULL) {
    node_ = new Node();
    node_->ptr = ptr;
    node_->ref_count = 1;
  }
}

template<typename T> shared_ptr<T>& shared_ptr<T>::operator=(
    const shared_ptr<T>& other) {
  if (node_ != other.node_) {
    DecrementRefCount();
    node_ = other.node_;
    IncrementRefCount();
  }

  return *this;
}

template<typename T> inline T& shared_ptr<T>::operator*() const {
  CHECK(node_ != NULL);
  CHECK(node_->ptr != NULL);

  return *node_->ptr;
}

template<typename T> inline T* shared_ptr<T>::operator->() const {
  CHECK(node_ != NULL);
  CHECK(node_->ptr != NULL);

  return node_->ptr;
}

template<typename T> T* shared_ptr<T>::get() const {
  if (node_ == NULL) {
    return NULL;
  } else {
    return node_->ptr;
  }
}

template<typename T> void shared_ptr<T>::IncrementRefCount() {
  if (node_ != NULL) {
    ++node_->ref_count;
    CHECK_GE(node_->ref_count, 2);
  }
}

template<typename T> void shared_ptr<T>::DecrementRefCount() {
  if (node_ != NULL) {
    CHECK_GE(node_->ref_count, 1);

    --node_->ref_count;

    if (node_->ref_count == 0) {
      delete node_->ptr;
      delete node_;
    }

    node_ = NULL;
  }
}

template<typename T> inline shared_ptr<T> make_shared_ptr(T* ptr) {
  return shared_ptr<T>(ptr);
}

}  // namespace floating_temple

#endif  // BASE_SHARED_PTR_H_
