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

#include "chre.h"

#include <cinttypes>

namespace chre {
namespace app {

uint32_t gOneShotTimerHandle;
uint32_t gCyclicTimerHandle;
int gCyclicTimerCount;

bool timerWorldStart() {
  chreLog(CHRE_LOG_INFO, "Timer World! - App started on platform ID %" PRIx64,
      chreGetPlatformId());

  gOneShotTimerHandle = chreTimerSet(100000000 /* duration: 100ms */,
      &gOneShotTimerHandle /* data */,
      true /* oneShot */);
  gCyclicTimerHandle = chreTimerSet(150000000 /* duration: 400ms */,
      &gCyclicTimerHandle /* data */,
      false /* oneShot */);
  gCyclicTimerCount = 0;
  return true;
}

void timerWorldHandleEvent(uint32_t senderInstanceId,
                           uint16_t eventType,
                           const void *eventData) {
  switch (eventType) {
    case CHRE_EVENT_TIMER: {
      const uint32_t *timerHandle = static_cast<const uint32_t *>(eventData);
      if (*timerHandle == gOneShotTimerHandle) {
        chreLog(CHRE_LOG_INFO, "Timer World! - One shot timer event received");
      } else if (*timerHandle == gCyclicTimerHandle) {
        chreLog(CHRE_LOG_INFO, "Timer World! - Cyclic timer event received");
        gCyclicTimerCount++;
        if (gCyclicTimerCount > 1) {
          chreTimerCancel(gCyclicTimerHandle);
        }
      }
    }
    break;
    default:
      chreLog(CHRE_LOG_ERROR, "Timer World! - Unknown event received");
    break;
  }
}

void timerWorldStop() {
  chreLog(CHRE_LOG_INFO, "Timer World! - Stopping");
}

}  // namespace app
}  // namespace chre
