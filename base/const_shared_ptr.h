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

#ifndef BASE_CONST_SHARED_PTR_H_
#define BASE_CONST_SHARED_PTR_H_

#include "base/logging.h"
#include "base/shared_ptr.h"

namespace floating_temple {

// A reference-counted shared pointer implementation. The pointed-to object is
// read-only until it's deleted. The object is deleted when the last copy of the
// shared pointer is destroyed.
template<typename T> class const_shared_ptr {
 public:
  typedef T element_type;

  explicit const_shared_ptr(T* ptr = nullptr);
  const_shared_ptr(const const_shared_ptr<T>& other);
  const_shared_ptr(const shared_ptr<T>& other);
  ~const_shared_ptr();

  // ptr may be NULL.
  void reset(T* ptr);

  const_shared_ptr<T>& operator=(const const_shared_ptr<T>& other);
  const_shared_ptr<T>& operator=(const shared_ptr<T>& other);

  const T& operator*() const;
  const T* operator->() const;

  const T* get() const;

 private:
  typedef typename shared_ptr<T>::Node Node;

  void IncrementRefCount();
  void DecrementRefCount();

  // If the shared pointer is set to NULL, node_ will be NULL.
  Node* node_;
};

template<typename T> const_shared_ptr<T>::const_shared_ptr(T* ptr) {
  if (ptr == nullptr) {
    node_ = nullptr;
  } else {
    node_ = new Node();
    node_->ptr = ptr;
    node_->ref_count = 1;
  }
}

template<typename T> const_shared_ptr<T>::const_shared_ptr(
    const const_shared_ptr<T>& other)
    : node_(other.node_) {
  IncrementRefCount();
}

template<typename T> const_shared_ptr<T>::const_shared_ptr(
    const shared_ptr<T>& other)
    : node_(other.node_) {
  IncrementRefCount();
}

template<typename T> const_shared_ptr<T>::~const_shared_ptr() {
  DecrementRefCount();
}

template<typename T> void const_shared_ptr<T>::reset(T* ptr) {
  if (node_ != nullptr && node_->ptr == ptr) {
    return;
  }

  DecrementRefCount();

  if (ptr != nullptr) {
    node_ = new Node();
    node_->ptr = ptr;
    node_->ref_count = 1;
  }
}

template<typename T> const_shared_ptr<T>& const_shared_ptr<T>::operator=(
    const const_shared_ptr<T>& other) {
  if (node_ != other.node_) {
    DecrementRefCount();
    node_ = other.node_;
    IncrementRefCount();
  }

  return *this;
}

template<typename T> const_shared_ptr<T>& const_shared_ptr<T>::operator=(
    const shared_ptr<T>& other) {
  if (node_ != other.node_) {
    DecrementRefCount();
    node_ = other.node_;
    IncrementRefCount();
  }

  return *this;
}

template<typename T> inline const T& const_shared_ptr<T>::operator*() const {
  CHECK(node_ != nullptr);
  CHECK(node_->ptr != nullptr);

  return *node_->ptr;
}

template<typename T> inline const T* const_shared_ptr<T>::operator->() const {
  CHECK(node_ != nullptr);
  CHECK(node_->ptr != nullptr);

  return node_->ptr;
}

template<typename T> const T* const_shared_ptr<T>::get() const {
  if (node_ == nullptr) {
    return nullptr;
  } else {
    return node_->ptr;
  }
}

template<typename T> void const_shared_ptr<T>::IncrementRefCount() {
  if (node_ != nullptr) {
    ++node_->ref_count;
    CHECK_GE(node_->ref_count, 2);
  }
}

template<typename T> void const_shared_ptr<T>::DecrementRefCount() {
  if (node_ != nullptr) {
    CHECK_GE(node_->ref_count, 1);

    --node_->ref_count;

    if (node_->ref_count == 0) {
      delete node_->ptr;
      delete node_;
    }

    node_ = nullptr;
  }
}

template<typename T> inline const_shared_ptr<T> make_const_shared_ptr(T* ptr) {
  return const_shared_ptr<T>(ptr);
}

}  // namespace floating_temple

#endif  // BASE_CONST_SHARED_PTR_H_
