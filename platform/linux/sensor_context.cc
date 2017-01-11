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

void SensorContext::init() {
  // TODO: Implement this. Probably we would open some files provided to mock
  // sensor data. Perhaps from command-line arguemnts.
}

bool SensorContext::getSensors(DynamicVector<PlatformSensor> *sensors) {
  CHRE_ASSERT(sensors);

  // TODO: Implement this. Perhaps look at all sensor trace files provided and
  // return the list of sensor data available.
  return false;
}

bool PlatformSensor::updatePlatformSensorRequest(const SensorRequest& request) {
  // TODO: Implement this. Perhaps consider the request and start to pass in
  // sensor samples from mock sensor data once the sensor has transitioned to
  // being enabled. Maybe consider resampling input data if the provided mock
  // data rate is higher than requested.
  return false;
}

}  // namespace chre
