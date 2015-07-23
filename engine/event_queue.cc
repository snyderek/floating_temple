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

#include "engine/event_queue.h"

#include "base/logging.h"
#include "util/producer_consumer_queue.h"

namespace floating_temple {
namespace engine {

EventQueue::EventQueue()
    : events_(-1),
      next_event_(nullptr),
      end_of_sequence_(false) {
}

EventQueue::~EventQueue() {
}

void EventQueue::QueueEvent(const CommittedEvent* event) {
  CHECK(event != nullptr);
  CHECK(events_.Push(event, true));
}

void EventQueue::SetEndOfSequence() {
  CHECK(events_.Push(nullptr, true));
}

bool EventQueue::HasNext() const {
  return PrivateHasNext();
}

const CommittedEvent* EventQueue::PeekNext() const {
  CHECK(PrivateHasNext());
  return next_event_;
}

const CommittedEvent* EventQueue::GetNext() {
  CHECK(PrivateHasNext());
  const CommittedEvent* const event = next_event_;
  next_event_ = nullptr;
  return event;
}

void EventQueue::MoveToNextSequence() {
  CHECK(next_event_ == nullptr);
  CHECK(end_of_sequence_);

  end_of_sequence_ = false;
}

bool EventQueue::PrivateHasNext() const {
  FetchNext();
  return next_event_ != nullptr;
}

void EventQueue::FetchNext() const {
  if (!end_of_sequence_ && next_event_ == nullptr) {
    const CommittedEvent* event = nullptr;
    CHECK(events_.Pop(&event, true));

    if (event == nullptr) {
      end_of_sequence_ = true;
    } else {
      next_event_ = event;
    }
  }
}

}  // namespace engine
}  // namespace floating_temple
