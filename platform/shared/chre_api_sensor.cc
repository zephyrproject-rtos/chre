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

#include "chre/core/event_loop_manager.h"
#include "chre/core/sensors.h"
#include "chre_api/chre/sensor.h"

using chre::EventLoopManagerSingleton;
using chre::SensorType;
using chre::getSensorTypeFromUnsignedInt;

bool chreSensorFindDefault(uint8_t sensorType, uint32_t *handle) {
  SensorType validatedSensorType = getSensorTypeFromUnsignedInt(sensorType);
  return (validatedSensorType != SensorType::Unknown
      && EventLoopManagerSingleton::get()->getSensorRequestManager()
          .getSensorHandle(validatedSensorType, handle));
}
