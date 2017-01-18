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

#ifndef CORE_INCLUDE_CHRE_CORE_SENSORS_IMPL_H_
#define CORE_INCLUDE_CHRE_CORE_SENSORS_IMPL_H_

#include "chre/core/sensors.h"

namespace chre {

constexpr bool sensorModeIsActive(SensorMode sensorMode) {
  return (sensorMode == SensorMode::ActiveContinuous
      || sensorMode == SensorMode::ActiveOneShot);
}

constexpr size_t getSensorTypeArrayIndex(SensorType sensorType) {
  return static_cast<size_t>(sensorType) - 1;
}

constexpr size_t getSensorTypeCount() {
  // The number of valid entries in the SensorType enum (not including Unknown).
  return static_cast<size_t>(SensorType::SENSOR_TYPE_COUNT) - 1;
}

}  // namespace chre

#endif  // CORE_INCLUDE_CHRE_CORE_SENSORS_IMPL_H_
