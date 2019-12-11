/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef CHRE_CORE_SENSOR_TYPE_HELPERS_H_
#define CHRE_CORE_SENSOR_TYPE_HELPERS_H_

#include "chre/core/sensor_type.h"
#include "chre/platform/platform_sensor_type_helpers.h"

namespace chre {

/**
 * Exposes several static methods to assist in determining sensor information
 * from the sensor type.
 */
class SensorTypeHelpers : public PlatformSensorTypeHelpers {
 public:
  /**
   * @param sensorType The type of sensor to check
   * @return Whether this sensor is a one-shot sensor.
   */
  static bool isOneShot(uint8_t sensorType) {
    return getReportingMode(sensorType) == ReportingMode::OneShot;
  }

  /**
   * @param sensorType The type of sensor to check
   * @return Whether this sensor is an on-change sensor.
   */
  static bool isOnChange(uint8_t sensorType) {
    return getReportingMode(sensorType) == ReportingMode::OnChange;
  }

  /**
   * @param sensorType The type of sensor to check
   * @return Whether this sensor is a continuous sensor.
   */
  static bool isContinuous(uint8_t sensorType) {
    return getReportingMode(sensorType) == ReportingMode::Continuous;
  }

  /**
   * @param sensorType The type of sensor to check
   * @return true if this sensor is a vendor sensor.
   */
  static bool isVendorSensorType(uint8_t sensorType) {
    return sensorType >= CHRE_SENSOR_TYPE_VENDOR_START;
  }

  /**
   * @param sensorType The type of sensor to get the reporting mode for.
   * @return the reporting mode for this sensor.
   */
  static ReportingMode getReportingMode(uint8_t sensorType);

  /**
   * @param sensorType The type of sensor to check
   * @return Whether this sensor is calibrated.
   */
  static bool isCalibrated(uint8_t sensorType);

  /**
   * @param sensorType The type of sensor to get the bias event type for.
   * @param eventType A non-null pointer to the event type to populate
   * @return true if this sensor has a bias event type.
   */
  static bool getBiasEventType(uint8_t sensorType, uint16_t *eventType);

  /**
   * Determines the size needed to store the latest event from a sensor. Since
   * only on-change sensors have their latest events retained, only those
   * sensors will receive a non-zero value from this method.
   *
   * @param sensorType The sensorType of this sensor.
   * @return the memory size for an on-change sensor to store its last data
   *     event.
   */
  static size_t getLastEventSize(uint8_t sensorType);

  /**
   * @param sensorType The sensor type to obtain a string for.
   * @return A string representation of the sensor type.
   */
  static const char *getSensorTypeName(uint8_t sensorType);
};

}  // namespace chre

#endif  // CHRE_CORE_SENSOR_TYPE_HELPERS_H_