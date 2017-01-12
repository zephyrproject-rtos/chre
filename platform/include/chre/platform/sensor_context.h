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

#ifndef CHRE_PLATFORM_SENSOR_CONTEXT_H_
#define CHRE_PLATFORM_SENSOR_CONTEXT_H_

#include "chre/core/sensors.h"
#include "chre/target_platform/sensor_context_base.h"
#include "chre/util/dynamic_vector.h"
#include "chre/util/non_copyable.h"

namespace chre {

/**
 * Provides an interface to obtain a platform-independent description of a
 * sensor. The PlatformSensorBase is subclassed here to allow platforms to
 * inject their own storage for their implementation.
 */
class PlatformSensor : public PlatformSensorBase {
 public:
  /**
   * Constructs a platform sensor. All sensors must have a type and must be
   * supplied to this constructor.
   */
  PlatformSensor(SensorType sensorType);

  /**
   * Returns the SensorType for this sensor.
   *
   * @return The type of this sensor.
   */
  SensorType getSensorType();

  /**
   * Sets the configuration of this sensor. If the request differs from current
   * request the platform sensor will be updated.
   *
   * @param request The configuration of the platform sensor.
   * @return Returns true if the new configuration was set correctly or if no
   *         change was required.
   */
  bool setRequest(const SensorRequest& request);

 private:
  //! The type of this sensor.
  SensorType mSensorType;

  //! The most recent sensor request sent to this sensor.
  SensorRequest mSensorRequest;

  /**
   * Sends the sensor request to the platform implementation. The implementation
   * of this method is supplied by the platform and is invoked when the current
   * request for this sensor changes.
   *
   * @param request The new request to set this sensor to.
   * @return Returns true if the platform sensor was successfully configured
   *         with the current request.
   */
  bool updatePlatformSensorRequest(const SensorRequest& request);
};

/**
 * Provides a mechanism to interact with sensors provided by the platform. This
 * includes requesting sensor data and querying available sensors.
 *
 * TODO: Refactor these static functions into PlatformSensor and remove this
 * class.
 */
class SensorContext {
 public:
  /**
   * Initializes the platform sensors subsystem. This must be called as part of
   * the initialization of the runtime.
   */
  static void init();

  /**
   * Obtains a list of the sensors that the platform provides. The supplied
   * DynamicVector should be empty when passed in. If this method returns false
   * the vector may be partially filled.
   *
   * @param sensors A non-null pointer to a DynamicVector to populate with the
   *                list of sensors.
   * @return Returns true if the query was successful.
   */
  static bool getSensors(DynamicVector<PlatformSensor> *sensors);

  /*
   * Deinitializes the platform sensors subsystem. This must be called as part
   * of the deinitialization of the runtime.
   */
  static void deinit();
};

}  // namespace chre

#endif  // CHRE_PLATFORM_SENSOR_CONTEXT_H_
