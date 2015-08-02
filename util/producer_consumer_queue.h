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

#ifndef UTIL_PRODUCER_CONSUMER_QUEUE_H_
#define UTIL_PRODUCER_CONSUMER_QUEUE_H_

#include <queue>

#include "base/cond_var.h"
#include "base/integral_types.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/mutex.h"
#include "base/mutex_lock.h"

namespace floating_temple {

// TODO(dss): Consider implementing this class using atomic compare-and-swap
// operations, instead of a mutex.
template<typename T>
class ProducerConsumerQueue {
 public:
  // If 'max_size' is -1, the queue size will be unlimited. Otherwise,
  // 'max_size' must be positive.
  explicit ProducerConsumerQueue(int max_size);
  ~ProducerConsumerQueue();

  // Attempts to add an item to the tail of the queue:
  //
  //   1) If the queue is being drained, this method will fail.
  //
  //   2) If the queue is not being drained and 'wait' is false, this method
  //      will succeed if the queue is not full.
  //
  //   3) If the queue is not being drained and 'wait' is true, this method will
  //      block until the queue is not full or Drain() is called. It will
  //      succeed if Drain() is not called.
  //
  // Returns true if the item was successfully added to the queue.
  bool Push(const T& item, bool wait);

  // Attempts to remove the item at the head of the queue:
  //
  //   1) If 'wait' is false, this method will succeed if the queue is not
  //      empty.
  //
  //   2) If 'wait' is true and the queue is being drained, this method will
  //      succeed if the queue is not empty.
  //
  //   3) If 'wait' is true and the queue is not being drained, this method will
  //      block until the queue is not empty or Drain() is called. It will
  //      succeed if the queue is not empty.
  //
  // 'item' must not be NULL. If the method succeeds, the removed item will be
  // assigned to *item. If the method fails, *item will be left unchanged.
  //
  // Returns true if an item was successfully removed from the queue.
  bool Pop(T* item, bool wait);

  // Puts the queue in draining mode. In draining mode, calls to Push() fail and
  // calls to Pop() succeed only if there are items left in the queue.
  void Drain();

 private:
  // Returns true if the queue is full. 'mu_' must be locked.
  bool QueueFull_Locked() const;

  const int max_size_;
  std::queue<T> queue_;
  bool draining_;
  mutable Mutex mu_;
  mutable CondVar queue_not_empty_cond_;
  mutable CondVar queue_not_full_cond_;

  DISALLOW_COPY_AND_ASSIGN(ProducerConsumerQueue);
};

template<typename T>
ProducerConsumerQueue<T>::ProducerConsumerQueue(int max_size)
    : max_size_(max_size),
      draining_(false) {
  if (max_size != -1) {
    CHECK_GT(max_size, 0);
  }
}

template<typename T>
ProducerConsumerQueue<T>::~ProducerConsumerQueue() {
}

template<typename T>
bool ProducerConsumerQueue<T>::Push(const T& item, bool wait) {
  MutexLock lock(&mu_);

  if (wait) {
    while (!draining_ && QueueFull_Locked()) {
      queue_not_full_cond_.Wait(&mu_);
    }
  }

  if (draining_) {
    return false;
  }

  queue_.push(item);
  queue_not_empty_cond_.Signal();

  return true;
}

template<typename T>
bool ProducerConsumerQueue<T>::Pop(T* item, bool wait) {
  CHECK(item != nullptr);

  MutexLock lock(&mu_);

  if (wait) {
    while (!draining_ && queue_.empty()) {
      queue_not_empty_cond_.WaitPatiently(&mu_);
    }
  }

  if (queue_.empty()) {
    return false;
  }

  *item = queue_.front();
  queue_.pop();
  queue_not_full_cond_.Signal();

  return true;
}

template<typename T>
void ProducerConsumerQueue<T>::Drain() {
  MutexLock lock(&mu_);
  draining_ = true;
  queue_not_empty_cond_.Broadcast();
  queue_not_full_cond_.Broadcast();
}

template<typename T>
bool ProducerConsumerQueue<T>::QueueFull_Locked() const {
  return max_size_ > 0 &&
      queue_.size() >=
      static_cast<typename std::queue<T>::size_type>(max_size_);
}

}  // namespace floating_temple

#endif  // UTIL_PRODUCER_CONSUMER_QUEUE_H_
