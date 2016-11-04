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

#ifndef CHRE_CORE_TIMER_H_
#define CHRE_CORE_TIMER_H_

/**
 * @file
 *
 */

// TODO: common timer module
//  - provide callback interface, build delayed event capability on top
//  - eventually, condense to single system timer (i.e. one that fires next),
//    but for now, can map 1:1 into system timer
//  - collection of pending timer events (list initially, but priority queue would be nice)

#include "chre/platform/system_timer.h"

namespace chre {

// Note that this mirrors the CHRE API definition of a timer handle, so should
// not be changed without appropriate consideration.
typedef uint32_t TimerHandle;

class Timer {
 public:
  // TODO: static method to invoke a callback after the specified time has elapsed.
  // the callback should be invoked within the context of the calling taskrunner.
  // this is the underlying API we would use to implement the corresponding CHRE API
  static TimerHandle setTimer(uint64_t delayNs, uint64_t intervalNs=0);

  // TODO: should also add methods here to:
  //   - post an event after a delay
  //   - invoke a callback in "unsafe" context (i.e. from other thread), which the
  //     CHRE system could use to trigger things while the task runner is busy

 private:
  // TODO: eventually, just use one SystemTimer on the bottom end here
};

}  // namespace chre

#endif  // CHRE_CORE_TIMER_H_
