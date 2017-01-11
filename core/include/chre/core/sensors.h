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
#include "chre/util/time.h"

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

/**
 * This SensorMode is designed to wrap constants provided by the CHRE API to
 * imrpove type-safety. The details of these modes are left to the CHRE API mode
 * definitions contained in the chreSensorConfigureMode enum.
 */
enum class SensorMode : uint8_t {
  Off               = CHRE_SENSOR_CONFIGURE_MODE_DONE,
  ActiveContinuous  = CHRE_SENSOR_CONFIGURE_MODE_CONTINUOUS,
  ActiveOneShot     = CHRE_SENSOR_CONFIGURE_MODE_ONE_SHOT,
  PassiveContinuous = CHRE_SENSOR_CONFIGURE_MODE_PASSIVE_CONTINUOUS,
  PassiveOneShot    = CHRE_SENSOR_CONFIGURE_MODE_PASSIVE_ONE_SHOT,
};

/**
 * @return Returns true if the sensor mode is considered to be active and would
 * cause a sensor to be powered on in order to get sensor data.
 */
constexpr bool sensorModeIsActive(SensorMode sensorMode);

/**
 * Models a request for sensor data. This class implements the API set forth by
 * the RequestMultiplexer container.
 */
class SensorRequest {
 public:
  /**
   * Default constructs a sensor request to the minimal possible configuration.
   * The sensor is disabled and the interval and latency are both set to zero.
   */
  SensorRequest();

  /**
   * Constructs a sensor request given a mode, interval and latency.
   *
   * @param mode The mode of the sensor request.
   * @param interval The interval between samples.
   * @param latency The maximum amount of time to batch samples before
   *        delivering to a client.
   */
  SensorRequest(SensorMode mode, Nanoseconds interval, Nanoseconds latency);

  /**
   * Performs an equivalency comparison of two sensor requests. This determines
   * if the effective request for sensor data is the same as another.
   *
   * @param request The request to compare against.
   * @return Returns true if this request is equivalent to another.
   */
  bool isEquivalentTo(const SensorRequest& request) const;

  /**
   * Generates a maximal intersection of this request and another and returns a
   * request that contains the superset of the mode, rate and latency of this
   * request.
   *
   * @param request The other request to compare the attributes of.
   * @return Returns a request that contains attributes that are the maximal of
   *         this request and the provided request.
   */
  SensorRequest generateIntersectionOf(const SensorRequest& request) const;

  /**
   * @return Returns the interval of samples for this request.
   */
  Nanoseconds getInterval() const;

  /**
   * @return Returns the maximum amount of time samples can be batched prior to
   * dispatching to the client.
   */
  Nanoseconds getLatency() const;

  /**
   * @return The mode of this request.
   */
  SensorMode getMode() const;

 private:
  //! The interval between samples for this request.
  Nanoseconds mInterval;

  //! The maximum amount of time samples can be batched prior to dispatching to
  //! the client
  Nanoseconds mLatency;

  //! The mode of this request.
  SensorMode mMode;
};

}  // namespace chre

#endif  // CHRE_CORE_SENSOR_H_
