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

 private:
  //! The type of this sensor.
  SensorType mSensorType;
};

/**
 * Provides a mechanism to interact with sensors provided by the platform. This
 * includes requesting sensor data and querying available sensors.
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
};

}  // namespace chre

#endif  // CHRE_PLATFORM_SENSOR_CONTEXT_H_
