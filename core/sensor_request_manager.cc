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

#include "chre/platform/fatal_error.h"

namespace chre {

SensorRequestManager::SensorRequestManager() {
  mSensorRequests.resize(mSensorRequests.capacity());

  DynamicVector<PlatformSensor> platformSensors;
  if (!PlatformSensor::getSensors(&platformSensors)) {
    LOGE("Failed to query the platform for sensors");
    return;
  }

  if (platformSensors.empty()) {
    LOGW("Platform returned zero sensors");
  }

  for (size_t i = 0; i < platformSensors.size(); i++) {
    PlatformSensor& platformSensor = platformSensors[i];
    SensorType sensorType = platformSensor.getSensorType();
    size_t sensorIndex = getSensorTypeArrayIndex(sensorType);
    LOGD("Found sensor: %s", getSensorTypeName(sensorType));

    mSensorRequests[sensorIndex].sensor = Sensor(platformSensor);
  }
}

SensorRequestManager::~SensorRequestManager() {
  SensorRequest nullRequest = SensorRequest();
  for (size_t i = 0; i < mSensorRequests.size(); i++) {
    // Disable sensors that have been enabled previously.
    Sensor& sensor = mSensorRequests[i].sensor;
    sensor.setRequest(nullRequest);
  }
}

bool SensorRequestManager::getSensorHandle(SensorType sensorType,
                                           uint32_t *sensorHandle) const {
  CHRE_ASSERT(sensorHandle);

  bool sensorHandleIsValid = false;
  if (sensorType == SensorType::Unknown) {
    LOGW("Querying for unknown sensor type");
  } else {
    size_t sensorIndex = getSensorTypeArrayIndex(sensorType);
    sensorHandleIsValid = mSensorRequests[sensorIndex].sensor.isValid();
    if (sensorHandleIsValid) {
      *sensorHandle = getSensorHandleFromSensorType(sensorType);
    }
  }

  return sensorHandleIsValid;
}

bool SensorRequestManager::setSensorRequest(Nanoapp *nanoapp,
    uint32_t sensorHandle, const SensorRequest& sensorRequest) {
  CHRE_ASSERT(nanoapp);

  // Validate the input to ensure that a valid handle has been provided.
  SensorType sensorType = getSensorTypeFromSensorHandle(sensorHandle);
  if (sensorType == SensorType::Unknown) {
    LOGW("Attempting to configure an invalid handle");
    return false;
  }

  // Ensure that the runtime is aware of this sensor type.
  size_t sensorIndex = getSensorTypeArrayIndex(sensorType);
  SensorRequests& requests = mSensorRequests[sensorIndex];
  if (!requests.sensor.isValid()) {
    LOGW("Attempting to configure non-existent sensor");
    return false;
  }

  Sensor& sensor = requests.sensor;
  uint16_t eventType = getSampleEventTypeForSensorType(sensor.getSensorType());

  size_t requestIndex = SIZE_MAX;
  const SensorRequest *previousSensorRequest =
      getSensorRequestForNanoapp(requests, nanoapp, &requestIndex);

  bool requestChanged = false;
  if (sensorRequest.getMode() == SensorMode::Off
      && previousSensorRequest != nullptr) {
    // The request changes the mode to off and there was an existing request.
    // The existing request is removed from the multiplexer.
    nanoapp->unregisterForBroadcastEvent(eventType);
    requests.multiplexer.removeRequest(requestIndex, &requestChanged);
  } else if (sensorRequest.getMode() != SensorMode::Off
      && previousSensorRequest == nullptr) {
    // The request changes the mode to the enabled state and there was no
    // existing request. The request is newly created and added to the
    // multiplexxer.
    nanoapp->registerForBroadcastEvent(eventType);
    requests.multiplexer.addRequest(sensorRequest, &requestChanged);
    requestIndex = (requests.multiplexer.getRequests().size() - 1);
  } else if (sensorRequest.getMode() != SensorMode::Off
      && previousSensorRequest != nullptr) {
    // The request changes the mode to the enabled state and there was an
    // existing request. The existing request is updated.
    requests.multiplexer.updateRequest(requestIndex, sensorRequest,
                                       &requestChanged);
  }

  if (requestChanged) {
    auto maximalRequest = requests.multiplexer.getCurrentMaximalRequest();
    requestChanged = sensor.setRequest(maximalRequest);
    if (!requestChanged) {
      // If the new maximal cannot be sent to the platform, roll the multiplexer
      // back. This behavior relies on the platform sensor request not changing
      // when a new request fails (ie: it will continue to sample with the
      // previous request configuration).
      //
      // This failure can only happen on the add/update request path (ie: not
      // when unsubscribing to a sensor request). This is considered a
      // FATAL_ERROR if the platform sensor cannot handle a request that was
      // already sent previously.
      if (requestIndex == SIZE_MAX) {
        FATAL_ERROR("Error rolling back the sensor request multiplexer");
      } else {
        // We remove the bad request from the multiplexer. The request will fall
        // back to the previous maximal request and there is no need to reset
        // the platform sensor.
        requests.multiplexer.removeRequest(requestIndex, &requestChanged);
      }
    }
  }

  return requestChanged;
}

const SensorRequest *SensorRequestManager::getSensorRequestForNanoapp(
    const SensorRequests& requests, const Nanoapp *nanoapp,
    size_t *index) const {
  CHRE_ASSERT(nanoapp);
  CHRE_ASSERT(index);

  const auto& requests_list = requests.multiplexer.getRequests();
  for (size_t i = 0; i < requests_list.size(); i++) {
    const SensorRequest& sensorRequest = requests_list[i];
    if (sensorRequest.getNanoapp() == nanoapp) {
      *index = i;
      return &sensorRequest;
    }
  }

  return nullptr;
}

}  // namespace chre
