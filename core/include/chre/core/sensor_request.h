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

#ifndef CHRE_CORE_SENSOR_REQUEST_H_
#define CHRE_CORE_SENSOR_REQUEST_H_

#include <cstdint>

#include "chre_api/chre/sensor.h"
#include "chre/core/nanoapp.h"
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
  Unknown,
  Accelerometer,
  InstantMotion,
  StationaryDetect,
  Gyroscope,
  GeomagneticField,
  Pressure,
  Light,
  Proximity,

  // Note to future developers: don't forget to update the implementation of
  // getSensorTypeName and getSensorTypeFromUnsignedInt when adding or removing
  // a new entry here :) Have a nice day.

  //! The number of sensor types including unknown. This entry must be last.
  SENSOR_TYPE_COUNT,
};

/**
 * Returns a string representation of the given sensor type.
 *
 * @param sensorType The sensor type to obtain a string for.
 * @return A string representation of the sensor type.
 */
const char *getSensorTypeName(SensorType sensorType);

/**
 * Returns a sensor sample event type for a given sensor type. The sensor type
 * must not be SensorType::Unknown. This is a fatal error.
 *
 * @param sensorType The type of the sensor.
 * @return The event type for a sensor sample of the given sensor type.
 */
uint16_t getSampleEventTypeForSensorType(SensorType sensorType);

/**
 * @return An index into an array for a given sensor type. This is useful to map
 * sensor type to array index quickly. The range returned corresponds to any
 * SensorType except for Unknown starting from zero to the maximum value sensor
 * with no gaps.
 */
constexpr size_t getSensorTypeArrayIndex(SensorType sensorType);

/**
 * @return The number of valid sensor types in the SensorType enum.
 */
constexpr size_t getSensorTypeCount();

/**
 * Translates an unsigned integer as provided by a CHRE-compliant nanoapp to a
 * SensorType. If the integer sensor type does not match one of the internal
 * sensor types, SensorType::Unknown is returned.
 *
 * @param sensorType The integer sensor type.
 * @return The strongly-typed sensor if a match is found or SensorType::Unknown.
 */
SensorType getSensorTypeFromUnsignedInt(uint8_t sensorType);

/**
 * Provides a stable handle for a CHRE sensor type. This handle is exposed to
 * CHRE nanoapps as a way to refer to sensors that they are subscribing to. This
 * API is not expected to be called with SensorType::Unknown as nanoapps are not
 * able to subscribe to the Unknown sensor type.
 *
 * @param sensorType The type of the sensor to obtain a handle for.
 * @return The handle for a given sensor.
 */
constexpr uint32_t getSensorHandleFromSensorType(SensorType sensorType);

/**
 * Maps a sensor handle to a SensorType or returns SensorType::Unknown if the
 * provided handle is invalid.
 *
 * @param handle The sensor handle for a sensor.
 * @return The sensor type for a given handle.
 */
constexpr SensorType getSensorTypeFromSensorHandle(uint32_t handle);

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

  //! The nanoapp that made this request. This will be nullptr when returned by
  //! the generateIntersectionOf method.
  Nanoapp *mNanoapp = nullptr;
};

}  // namespace chre

#include "chre/core/sensor_request_impl.h"

#endif  // CHRE_CORE_SENSOR_REQUEST_H_
