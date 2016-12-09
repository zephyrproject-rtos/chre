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

#include "chre/core/event_loop.h"
#include "chre/core/timer_pool.h"

namespace chre {

TimerPool::TimerPool(EventLoop& eventLoop)
    : mEventLoop(eventLoop) {}

TimerHandle TimerPool::setTimer(Nanoseconds duration, void *cookie,
    bool isOneShot) {
  bool mTimerIsActive = false; // TODO: Remove this.
  if (mTimerIsActive) {
    // TODO: Push the request onto the list of requests.
  } else {
    // TODO: Immediately request the timer and set mTimerIsActive to true.
  }

  // TODO: Implement this.
  return 0;
}

}  // namespace chre
