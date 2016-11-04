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

#ifndef CHRE_APPS_HELLO_WORLD_HELLO_WORLD_H_
#define CHRE_APPS_HELLO_WORLD_HELLO_WORLD_H_

#include <cstdint>

namespace chre {
namespace app {

/**
 * The primary entry point of a hello world nanoapp. This nanoapp starts, logs
 * in each entry point and stops. All events are logged.
 *
 * @return This app always returns true to indicate success.
 */
bool helloWorldStart();

/**
 * The handle event entry point for the hello world program. This nanoapp just
 * logs the event type and time when an event was received.
 *
 * @param the sender instance ID
 * @param the type of the event data
 * @param a pointer to the event data
 */
void helloWorldHandleEvent(uint32_t senderInstanceId,
                           uint16_t eventType,
                           const void *eventData);

/**
 * Stops the app. This nanoapp just logs that it is stopping.
 */
void helloWorldStop();

}  // namespace app
}  // namespace chre

#endif  // CHRE_APPS_HELLO_WORLD_HELLO_WORLD_H_
