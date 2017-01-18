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

#ifndef CHRE_APPS_SENSOR_WORLD_SENSOR_WORLD_H_
#define CHRE_APPS_SENSOR_WORLD_SENSOR_WORLD_H_

#include <cstdint>

namespace chre {
namespace app {

/**
 * The primary entry point of a sensor world nanoapp. This nanoapp starts and
 * requests details about CHRE sensors and logs the results.
 *
 * @return This app always returns true to indicate success.
 */
bool sensorWorldStart();

/**
 * The handle event entry point for the sensor world program.
 *
 * @param the sender instance ID
 * @param the type of the event data
 * @param a pointer to the event data
 */
void sensorWorldHandleEvent(uint32_t senderInstanceId,
                            uint16_t eventType,
                            const void *eventData);

/**
 * Stops the sensor world app.
 */
void sensorWorldStop();

}  // namespace app
}  // namespace chre

#endif  // CHRE_APPS_SENSOR_WORLD_SENSOR_WORLD_H_
