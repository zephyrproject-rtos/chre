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

#include "chre/platform/sensor_context.h"

namespace chre {

PlatformSensor::PlatformSensor()
    : PlatformSensor(SensorType::Unknown) {}

PlatformSensor::PlatformSensor(SensorType sensorType)
    : mSensorType(sensorType) {}

SensorType PlatformSensor::getSensorType() {
  return mSensorType;
}

bool PlatformSensor::setRequest(const SensorRequest& request) {
  bool configuredSuccessfully = true;
  if (!mSensorRequest.isEquivalentTo(request)) {
    configuredSuccessfully = updatePlatformSensorRequest(request);
    if (configuredSuccessfully) {
      mSensorRequest = request;
    }
  }

  return configuredSuccessfully;
}

}  // namespace chre
