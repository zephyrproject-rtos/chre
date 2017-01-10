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

#ifndef CHRE_CORE_SENSOR_H_
#define CHRE_CORE_SENSOR_H_

#include <cstdint>

#include "chre_api/chre/sensor.h"

namespace chre {

/**
 * This SensorType is designed to wrap constants provided by the CHRE API
 * to improve type-safety. In addition, an unknown sensor type is provided
 * for dealing with sensors that are not defined as per the CHRE API
 * specification. The details of these sensors are left to the CHRE API
 * sensor definitions.
 */
enum class SensorType : uint8_t {
  Unknown          = 0,
  Accelerometer    = CHRE_SENSOR_TYPE_ACCELEROMETER,
  InstantMotion    = CHRE_SENSOR_TYPE_INSTANT_MOTION_DETECT,
  StationaryDetect = CHRE_SENSOR_TYPE_STATIONARY_DETECT,
  Gyroscope        = CHRE_SENSOR_TYPE_GYROSCOPE,
  GeomagneticField = CHRE_SENSOR_TYPE_GEOMAGNETIC_FIELD,
  Pressure         = CHRE_SENSOR_TYPE_PRESSURE,
  Light            = CHRE_SENSOR_TYPE_LIGHT,
  Proximity        = CHRE_SENSOR_TYPE_PROXIMITY,

  // Note to future developers: don't forget to update the implementation of
  // getSensorTypeName when adding a new entry here :) Have a nice day.
};

/**
 * Returns a string representation of the given sensor type.
 *
 * @param sensorType The sensor type to obtain a string for.
 * @return A string representation of the sensor type.
 */
const char *getSensorTypeName(SensorType sensorType);

}  // namespace chre

#endif  // CHRE_CORE_SENSOR_H_
