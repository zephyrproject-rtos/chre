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

#include "chre/core/sensor_request_manager.h"

namespace chre {

SensorRequestManager::SensorRequestManager() {
  mSensorRequests.resize(mSensorRequests.capacity());

  DynamicVector<PlatformSensor> platformSensors;
  if (!SensorContext::getSensors(&platformSensors)) {
    LOGE("Failed to query the platform for sensors");
    return;
  }

  if (platformSensors.empty()) {
    LOGW("Platform returned zero sensors");
  }

  for (size_t i = 0; i < platformSensors.size(); i++) {
    PlatformSensor& sensor = platformSensors[i];
    size_t sensorIndex = getSensorTypeArrayIndex(sensor.getSensorType());
    LOGD("SensorRequestManager found sensor: %s",
         getSensorTypeName(sensor.getSensorType()));
    mSensorRequests[sensorIndex].sensor = std::move(sensor);
  }
}

SensorRequestManager::~SensorRequestManager() {
  for (size_t i = 0; i < mSensorRequests.size(); i++) {
    // Disable sensors that have been enabled previously.
    Optional<PlatformSensor>& sensor = mSensorRequests[i].sensor;
    if (sensor.has_value()) {
      sensor->setRequest(SensorRequest());
    }
  }
}

bool SensorRequestManager::getSensorHandle(SensorType sensorType,
                                           uint32_t *sensorHandle) const {
  CHRE_ASSERT(sensorHandle);

  bool hasSensor = false;
  if (sensorType == SensorType::Unknown) {
    LOGW("Querying for unknown sensor type");
  } else {
    size_t sensorIndex = getSensorTypeArrayIndex(sensorType);
    hasSensor = mSensorRequests[sensorIndex].sensor.has_value();
    if (hasSensor) {
      // The sensorIndex obtained above will always be small so we do not need
      // to be concerned about integer overflow here. We increment by one to
      // avoid handles of zero being valid. Whenever a client provides a handle
      // we must decrement by one.
      *sensorHandle = static_cast<uint32_t>(sensorIndex) + 1;
    }
  }

  return hasSensor;
}

}  // namespace chre
