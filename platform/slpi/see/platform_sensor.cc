/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "chre/platform/platform_sensor.h"

#include "chre_api/chre/sensor.h"
#include "chre/core/event_loop_manager.h"
#include "chre/core/sensor.h"
#include "chre/platform/assert.h"
#include "chre/platform/fatal_error.h"
#include "chre/platform/log.h"
#include "chre/platform/system_time.h"

namespace chre {

PlatformSensor::~PlatformSensor() {
  if (lastEvent != nullptr) {
    LOGD("Releasing lastEvent: 0x%p, size %zu",
         lastEvent, lastEventSize);
    memoryFree(lastEvent);
  }
}

void PlatformSensor::init() {
  // TODO: Implement this.
}

void PlatformSensor::deinit() {
  // TODO: Implement this.
}

bool PlatformSensor::getSensors(DynamicVector<Sensor> *sensors) {
  CHRE_ASSERT(sensors);

  // TODO: Implement this.
  return false;
}

bool PlatformSensor::applyRequest(const SensorRequest& request) {
  // TODO: Implement this.
  return false;
}

SensorType PlatformSensor::getSensorType() const {
  // TODO: Implement this.
  return SensorType::Unknown;
}

uint64_t PlatformSensor::getMinInterval() const {
  return minInterval;
}

const char *PlatformSensor::getSensorName() const {
  return sensorName;
}

PlatformSensor::PlatformSensor(PlatformSensor&& other) {
  // Our move assignment operator doesn't assume that "this" is initialized, so
  // we can just use that here.
  *this = std::move(other);
}

PlatformSensor& PlatformSensor::operator=(PlatformSensor&& other) {
  // TODO: Implement this.
  // Note: if this implementation is ever changed to depend on "this" containing
  // initialized values, the move constructor implemenation must be updated.
  memcpy(sensorName, other.sensorName, 1);
  minInterval = other.minInterval;

  lastEvent = other.lastEvent;
  other.lastEvent = nullptr;

  lastEventSize = other.lastEventSize;
  other.lastEventSize = 0;

  lastEventValid = other.lastEventValid;
  isSensorOff = other.isSensorOff;
  samplingStatus = other.samplingStatus;

  return *this;
}

ChreSensorData *PlatformSensor::getLastEvent() const {
  return (this->lastEventValid) ? this->lastEvent : nullptr;
}

bool PlatformSensor::getSamplingStatus(
    struct chreSensorSamplingStatus *status) const {
  CHRE_ASSERT(status);

  bool success = false;
  if (status != nullptr) {
    success = true;
    memcpy(status, &samplingStatus, sizeof(*status));
  }
  return success;
}

void PlatformSensorBase::setLastEvent(const ChreSensorData *event) {
  memcpy(this->lastEvent, event, this->lastEventSize);
  this->lastEventValid = true;
}

}  // namespace chre
