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

#ifndef CHRE_PLATFORM_LINUX_SYSTEM_TIMER_BASE_H_
#define CHRE_PLATFORM_LINUX_SYSTEM_TIMER_BASE_H_

/**
 * @file
 * POSIX implementation of the CHRE timer abstraction
 */

#include <signal.h>
#include <time.h>

namespace chre {

struct SystemTimerBase {
  timer_t mTimerId;

  static void systemTimerNotifyCallback(union sigval cookie);

  bool setInternal(uint64_t delayNs, uint64_t intervalNs);
};

}

#endif  // CHRE_PLATFORM_LINUX_SYSTEM_TIMER_BASE_H_
