/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "EventQueue.h"

#include "EventEmitter.h"

namespace facebook {
namespace react {

EventQueue::EventQueue(const EventPipe &eventPipe, std::unique_ptr<EventBeat> eventBeat):
  eventPipe_(eventPipe),
  eventBeat_(std::move(eventBeat)) {
    eventBeat_->setBeatCallback(std::bind(&EventQueue::onBeat, this));
  }

void EventQueue::enqueueEvent(const RawEvent &rawEvent) const {
  std::lock_guard<std::mutex> lock(queueMutex_);
  queue_.push_back(rawEvent);
}

void EventQueue::onBeat() const {
  std::vector<RawEvent> queue;

  {
    std::lock_guard<std::mutex> lock(queueMutex_);

    if (queue_.size() == 0) {
      return;
    }

    queue = std::move(queue_);
    assert(queue_.size() == 0);
  }

  {
    std::lock_guard<std::recursive_mutex> lock(EventEmitter::DispatchMutex());
    for (const auto &event : queue) {
      eventPipe_(
        event.isDispachable() ? event.eventTarget : EmptyEventTarget,
        event.type,
        event.payload
      );
    }
  }
}

} // namespace react
} // namespace facebook
