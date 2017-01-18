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

#include "chre/util/array.h"

namespace chre {
namespace app {

struct SensorState {
  const uint8_t type;
  uint32_t handle;
  bool isInitialized;
};

SensorState sensors[] = {
  { .type = CHRE_SENSOR_TYPE_ACCELEROMETER, },
  { .type = CHRE_SENSOR_TYPE_INSTANT_MOTION_DETECT, },
  { .type = CHRE_SENSOR_TYPE_STATIONARY_DETECT, },
  { .type = CHRE_SENSOR_TYPE_GYROSCOPE, },
  { .type = CHRE_SENSOR_TYPE_GEOMAGNETIC_FIELD, },
  { .type = CHRE_SENSOR_TYPE_PRESSURE, },
  { .type = CHRE_SENSOR_TYPE_LIGHT, },
  { .type = CHRE_SENSOR_TYPE_PROXIMITY, },
};

void getSensorAndLog(uint8_t sensorType, uint32_t *sensorHandle) {
  bool success = chreSensorFindDefault(sensorType, sensorHandle);
  chreLog(CHRE_LOG_INFO,
          "chreSensorFindDefault returned %s with handle %" PRIu32,
          success ? "true" : "false", *sensorHandle);
}

bool sensorWorldStart() {
  chreLog(CHRE_LOG_INFO, "Sensor World! - App started on platform ID %" PRIx64,
          chreGetPlatformId());

  for (size_t i = 0; i < ARRAY_SIZE(sensors); i++) {
    sensors[i].isInitialized = chreSensorFindDefault(sensors[i].type,
                                                     &sensors[i].handle);
    chreLog(CHRE_LOG_INFO, "sensor initialized: %s with handle %" PRIu32,
            sensors[i].isInitialized ? "true" : "false", sensors[i].handle);
  }

  return true;
}

void sensorWorldHandleEvent(uint32_t senderInstanceId,
                           uint16_t eventType,
                           const void *eventData) {
  uint64_t currentTime = chreGetTime();
  chreLog(CHRE_LOG_INFO, "Sensor World! - Received event 0x%" PRIx16
          " at time %" PRIu64, eventType, currentTime);
}

void sensorWorldStop() {
  chreLog(CHRE_LOG_INFO, "Sensor World! - Stopped");
}

}  // namespace app
}  // namespace chre
