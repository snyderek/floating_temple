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

#ifndef UTIL_QUOTA_QUEUE_H_
#define UTIL_QUOTA_QUEUE_H_

#include <cstddef>
#include <utility>
#include <vector>

#include "base/cond_var.h"
#include "base/linked_ptr.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/mutex.h"
#include "base/mutex_lock.h"
#include "util/producer_consumer_queue.h"

namespace floating_temple {

template<typename T>
class QuotaQueue {
 public:
  // If max_size is -1, the queue size will be unlimited.
  explicit QuotaQueue(int max_size);
  ~QuotaQueue();

  void AddService(int service_id, int max_item_count);

  bool Push(const T& item, int service_id, bool wait);
  bool Pop(T* item, int* service_id, bool wait);

  void Drain();

 private:
  struct Service {
    int max_item_count;
    int item_count;
    mutable CondVar service_not_full_cond;
  };

  Service* GetService_Locked(int service_id);
  bool ServiceFull_Locked(const Service* service) const;

  std::vector<linked_ptr<Service> > services_;
  bool draining_;
  mutable Mutex mu_;

  ProducerConsumerQueue<std::pair<T, int> > queue_;

  DISALLOW_COPY_AND_ASSIGN(QuotaQueue);
};

template<typename T>
QuotaQueue<T>::QuotaQueue(int max_size)
    : draining_(false),
      queue_(max_size) {
}

template<typename T>
QuotaQueue<T>::~QuotaQueue() {
}

template<typename T>
void QuotaQueue<T>::AddService(int service_id, int max_item_count) {
  CHECK_GE(service_id, 0);

  if (max_item_count != -1) {
    CHECK_GT(max_item_count, 0);
  }

  Service* const new_service = new Service();
  new_service->max_item_count = max_item_count;
  new_service->item_count = 0;

  {
    MutexLock lock(&mu_);

    const typename std::vector<linked_ptr<Service> >::size_type new_index =
        static_cast<typename std::vector<linked_ptr<Service> >::size_type>(
            service_id);

    if (new_index >= services_.size()) {
      services_.resize(new_index + 1u);
    }

    linked_ptr<Service>& service_ptr = services_[service_id];
    CHECK(service_ptr.get() == NULL);
    service_ptr.reset(new_service);
  }
}

template<typename T>
bool QuotaQueue<T>::Push(const T& item, int service_id, bool wait) {
  {
    MutexLock lock(&mu_);

    Service* const service = GetService_Locked(service_id);
    if (wait) {
      while (!draining_ && ServiceFull_Locked(service)) {
        service->service_not_full_cond.Wait(&mu_);
      }
    }

    if (draining_) {
      return false;
    }

    ++service->item_count;
  }

  return queue_.Push(std::make_pair(item, service_id), wait);
}

template<typename T>
bool QuotaQueue<T>::Pop(T* item, int* service_id, bool wait) {
  CHECK(item != NULL);
  CHECK(service_id != NULL);

  std::pair<T, int> item_pair;

  if (!queue_.Pop(&item_pair, wait)) {
    return false;
  }

  *item = item_pair.first;
  *service_id = item_pair.second;

  {
    MutexLock lock(&mu_);

    Service* const service = GetService_Locked(*service_id);

    --service->item_count;
    CHECK_GE(service->item_count, 0);

    service->service_not_full_cond.Signal();
  }

  return true;
}

template<typename T>
void QuotaQueue<T>::Drain() {
  {
    MutexLock lock(&mu_);

    draining_ = true;

    for (typename std::vector<linked_ptr<Service> >::iterator it =
             services_.begin();
         it != services_.end(); ++it) {
      Service* const service = it->get();

      if (service != NULL) {
        service->service_not_full_cond.Broadcast();
      }
    }
  }

  queue_.Drain();
}

template<typename T>
typename QuotaQueue<T>::Service* QuotaQueue<T>::GetService_Locked(
    int service_id) {
  CHECK_GE(service_id, 0);
  CHECK_LT(service_id, services_.size());

  Service* const service = services_[service_id].get();
  CHECK(service != NULL);

  return service;
}

template<typename T>
bool QuotaQueue<T>::ServiceFull_Locked(const Service* service) const {
  CHECK(service != NULL);

  const int max_item_count = service->max_item_count;
  return max_item_count > 0 && service->item_count >= max_item_count;
}

}  // namespace floating_temple

#endif  // UTIL_QUOTA_QUEUE_H_
