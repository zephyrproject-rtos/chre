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

#ifndef CHRE_PLATFORM_PLATFORM_SENSOR_H_
#define CHRE_PLATFORM_PLATFORM_SENSOR_H_

#include "chre/core/sensor_request.h"
#include "chre/core/sensor_type.h"
#include "chre/target_platform/platform_sensor_base.h"
#include "chre/util/dynamic_vector.h"

namespace chre {

/**
 * Defines the common interface to sensor functionality that is implemented in a
 * platform-specific way, and must be supported on every platform.
 *
 * @see Sensor
 */
class PlatformSensor : public PlatformSensorBase {
 public:
  /**
   * Obtains the SensorType of this platform sensor. The implementation of this
   * method is supplied by the platform as the mechanism for determining the
   * type may vary across platforms.
   *
   * @return The type of this sensor.
   */
  uint8_t getSensorType() const;

  /**
   * @return This sensor's minimum supported sampling interval, in nanoseconds.
   */
  uint64_t getMinInterval() const;

  /**
   * @return Whether this sensor reports bias events.
   */
  bool reportsBiasEvents() const;

  /**
   * Returns a descriptive name (e.g. type and model) for this sensor.
   *
   * @return A pointer to a string with storage duration at least as long as the
   *         lifetime of this object.
   */
  const char *getSensorName() const;

  /**
   * Gets the current status of this sensor in the CHRE API format.
   *
   * @param status A non-null pointer to chreSensorSamplingStatus to populate
   * @return true if the sampling status has been successfully obtained.
   */
  bool getSamplingStatus(struct chreSensorSamplingStatus *status) const;

  /**
   * Sets the current status of this sensor in the CHRE API format.
   *
   * @param status The current sampling status.
   */
  void setSamplingStatus(const struct chreSensorSamplingStatus &status);

 protected:
  struct chreSensorSamplingStatus mSamplingStatus;

  /**
   * Default constructor that puts this instance in an unspecified state.
   * Additional platform-specific initialization will likely be necessary to put
   * this object in a usable state. Do not construct PlatformSensor directly;
   * instead construct via Sensor.
   */
  PlatformSensor() = default;
};

}  // namespace chre

#endif  // CHRE_PLATFORM_PLATFORM_SENSOR_H_
