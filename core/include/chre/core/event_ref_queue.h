/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CHRE_EVENT_QUEUE_H
#define CHRE_EVENT_QUEUE_H

#include "chre/core/event.h"
#include "chre/platform/assert.h"

#include <deque>

namespace chre {

/**
 * TODO
 * NOT thread-safe, non-blocking.
 */
class EventRefQueue {
 public:
  bool empty() {
    return mQueue.empty();
  }

  bool push(Event *event) {
    CHRE_ASSERT(event != nullptr);

    mQueue.push_back(event);
    event->incrementRefCount();

    return true;
  }

  Event *pop() {
    CHRE_ASSERT(!mQueue.empty());

    Event *event = mQueue.front();
    mQueue.pop_front();
    event->decrementRefCount();

    return event;
  }

 private:
  std::deque<Event*> mQueue;
};

}  // namespace chre

#endif //CHRE_EVENT_QUEUE_H
