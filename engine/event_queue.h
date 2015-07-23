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

#ifndef ENGINE_EVENT_QUEUE_H_
#define ENGINE_EVENT_QUEUE_H_

#include "base/macros.h"
#include "util/producer_consumer_queue.h"

namespace floating_temple {
namespace engine {

class CommittedEvent;

class EventQueue {
 public:
  EventQueue();
  ~EventQueue();

  // These methods must only be called by the enqueuing thread.
  void QueueEvent(const CommittedEvent* event);
  void SetEndOfSequence();

  // These methods must only be called by the dequeuing thread.
  bool HasNext() const;
  const CommittedEvent* PeekNext() const;
  const CommittedEvent* GetNext();
  void MoveToNextSequence();

 private:
  // These methods are only called by the dequeuing thread.
  bool PrivateHasNext() const;
  void FetchNext() const;

  // This member variable is accessed by both threads.
  mutable ProducerConsumerQueue<const CommittedEvent*> events_;

  // These member variables are only accessed by the dequeuing thread.
  mutable const CommittedEvent* next_event_;
  mutable bool end_of_sequence_;

  DISALLOW_COPY_AND_ASSIGN(EventQueue);
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_EVENT_QUEUE_H_
