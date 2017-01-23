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

#ifndef CHRE_CORE_SENSOR_REQUEST_MANAGER_H_
#define CHRE_CORE_SENSOR_REQUEST_MANAGER_H_

#include "chre/core/request_multiplexer.h"
#include "chre/core/sensors.h"
#include "chre/platform/platform_sensor.h"
#include "chre/util/fixed_size_vector.h"
#include "chre/util/non_copyable.h"
#include "chre/util/optional.h"

namespace chre {

class SensorRequestManager : public NonCopyable {
 public:
  /**
   * Performs initialization of the SensorRequestManager and populates the
   * sensor list with platform sensors.
   */
  SensorRequestManager();

  /**
   * Destructs the sensor request manager and releases platform sensor resources
   * if requested.
   */
  ~SensorRequestManager();

  /**
   * Determines whether the runtime is aware of a given sensor type. The
   * supplied sensorHandle is only populated if the sensor type is known.
   *
   * @param sensorType The type of the sensor.
   * @param sensorHandle A non-null pointer to a uint32_t to use as a sensor
   *                     handle for nanoapps.
   * @return true if the supplied sensor type is available for use.
   */
  bool getSensorHandle(SensorType sensorType, uint32_t *sensorHandle) const;

 private:
  /**
   * Pairs a PlatformSensor with a RequestMultiplexer of SensorRequest. This
   * allows tracking the state of a platform sensor with the various requests
   * for it and can trigger a change in rate/latency when required.
   */
  struct SensorRequests {
    //! The platform sensor for this request manager if it was found during
    //! initialization.
    Optional<PlatformSensor> sensor;

    //! The request multiplexer for this sensor.
    RequestMultiplexer<SensorRequest> requestMultiplexer;
  };

  //! The list of sensor requests
  FixedSizeVector<SensorRequests, getSensorTypeCount()> mSensorRequests;
};

}  // namespace chre

#endif  // CHRE_CORE_SENSOR_REQUEST_MANAGER_H_
