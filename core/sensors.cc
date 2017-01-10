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

#include "chre/core/sensors.h"

#include "chre/platform/assert.h"

namespace chre {

const char *getSensorTypeName(SensorType sensorType) {
  switch (sensorType) {
    case SensorType::Unknown:
      return "Unknown";
    case SensorType::Accelerometer:
      return "Accelerometer";
    case SensorType::InstantMotion:
      return "Instant Motion";
    case SensorType::StationaryDetect:
      return "Stationary Detect";
    case SensorType::Gyroscope:
      return "Gyroscope";
    case SensorType::GeomagneticField:
      return "Geomagnetic Field";
    case SensorType::Pressure:
      return "Pressure";
    case SensorType::Light:
      return "Light";
    case SensorType::Proximity:
      return "Proximity";
    default:
      CHRE_ASSERT(false);
      return "";
  }
}

}  // namespace chre
