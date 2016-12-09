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

#ifndef CHRE_APPS_TIMER_WORLD_TIMER_WORLD_H_
#define CHRE_APPS_TIMER_WORLD_TIMER_WORLD_H_

#include <cstdint>

namespace chre {
namespace app {

/**
 * The primary entry point of the timer world nanoapp. This app is designed to
 * exercise timer logic by logging occasionally.
 *
 * @return This app returns true if a timer has been requested successfully.
 */
bool timerWorldStart();

/**
 * The handle event entry point for the timer world program.
 *
 * @param senderInstanceId The sender instance ID
 * @param eventType The type of the event data
 * @param eventData A pointer to the event data
 */

void timerWorldHandleEvent(uint32_t senderInstanceId,
                           uint16_t eventType,
                           const void *eventData);

/**
 * Stops the app.
 */
void timerWorldStop();

}  // namespace app
}  // namespace chre

#endif  // CHRE_APPS_TIMER_WORLD_TIMER_WORLD_H_
