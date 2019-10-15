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

#include "chre/core/sensor_type_helpers.h"
#include "chre/platform/platform_sensor.h"
#include "chre/util/non_copyable.h"
#include "chre/util/optional.h"

namespace chre {

/**
 * Represents a sensor in the system that is exposed to nanoapps in CHRE.
 *
 * Note that like chre::Nanoapp, this class uses inheritance to separate the
 * common code (Sensor) from common interface with platform-specific
 * implementation (PlatformSensor). However, this inheritance relationship does
 * *not* imply polymorphism, and this object must only be referred to via the
 * most-derived type, i.e. chre::Sensor.
 */
class Sensor : public PlatformSensor {
 public:
  /**
   * Constructs a sensor in an unspecified state. Should not be called directly
   * by common code, as platform-specific initialization of the Sensor object is
   * required for it to be usable.
   *
   * @see PlatformSensorManager::getSensors
   */
  Sensor() = default;

  /**
   * Obtains a reference to the latest request that has been accepted by the
   * platform.
   *
   * @return A const reference to the SensorRequest.
   */
  const SensorRequest &getRequest() const {
    return mSensorRequest;
  }

  /**
   * Sets the request of this sensor that's been accepted by the platform.
   *
   * @param request The new request for this sensor.
   */
  void setRequest(const SensorRequest &request) {
    mSensorRequest = request;
  }

  /**
   * @return Pointer to this sensor's last data event. It returns a nullptr if
   *         the sensor doesn't provide it.
   */
  ChreSensorData *getLastEvent() const {
    return mLastEvent;
  }

  /**
   * Sets the most recent event received for this sensor.
   *
   * @param event The most recent event received for this sensor.
   */
  void setLastEvent(ChreSensorData *event) {
    mLastEvent = event;
  }

  /**
   * @return Whether this sensor is a one-shot sensor.
   */
  bool isOneShot() const {
    return SensorTypeHelpers::isOneShot(getSensorType());
  }

  /**
   * @return Whether this sensor is an on-change sensor.
   */
  bool isOnChange() const {
    return SensorTypeHelpers::isOnChange(getSensorType());
  }

  /**
   * @return Whether this sensor is a continuous sensor.
   */
  bool isContinuous() const {
    return SensorTypeHelpers::isContinuous(getSensorType());
  }

  /**
   * @return Whether this sensor is calibrated.
   */
  bool isCalibrated() const {
    return SensorTypeHelpers::isCalibrated(getSensorType());
  }

  /**
   * @param eventType A non-null pointer to the event type to populate
   * @return true if this sensor has a bias event type.
   */
  bool getBiasEventType(uint16_t *eventType) const {
    return SensorTypeHelpers::getBiasEventType(getSensorType(), eventType);
  }

  /**
   * Gets the sensor info of this sensor in the CHRE API format.
   *
   * @param info A non-null pointer to the chreSensor info to populate.
   * @param targetApiVersion CHRE_API_VERSION_ value corresponding to the API
   *     that the info struct should be populated for.
   */
  void populateSensorInfo(struct chreSensorInfo *info,
                          uint32_t targetApiVersion) const;

 private:
  //! The most recent sensor request accepted by the platform.
  SensorRequest mSensorRequest;

  //! The most recent event received for this sensor;
  ChreSensorData *mLastEvent;
};

}  // namespace chre

#endif  // CHRE_CORE_SENSOR_H_
