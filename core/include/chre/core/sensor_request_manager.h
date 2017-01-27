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
#include "chre/core/sensor.h"
#include "chre/core/sensor_request.h"
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

  /**
   * Sets a sensor request for the given nanoapp for the provided sensor handle.
   * If the nanoapp has made a previous request, it is replaced by this request.
   * If the request changes the mode to SensorMode::Off the request is removed.
   *
   * @param nanoapp A non-null pointer to the nanoapp requesting this change.
   * @param sensorHandle The sensor handle for which this sensor request is
   *        directed at.
   * @param request The new sensor request for this nanoapp.
   * @return true if the request was set successfully. If the sensorHandle is
   *         out of range or the platform sensor fails to update to the new
   *         request false will be returned.
   */
  bool setSensorRequest(Nanoapp *nanoapp, uint32_t sensorHandle,
                        const SensorRequest& sensorRequest);

 private:
  /**
   * This allows tracking the state of a sensor with the various requests for it
   * and can trigger a change in rate/latency when required.
   */
  struct SensorRequests {
    //! The sensor associated with this request multiplexer.
    Sensor sensor;

    //! The request multiplexer for this sensor.
    RequestMultiplexer<SensorRequest> multiplexer;
  };

  //! The list of sensor requests
  FixedSizeVector<SensorRequests, getSensorTypeCount()> mSensorRequests;

  /**
   * Searches through a list of sensor requests for a previous sensor request
   * from the given nanoapp. The provided index pointer is populated with the
   * index of the request if it is found.
   *
   * @param requests The list of requests to search through.
   * @param nanoapp The nanoapp whose request is being searched for.
   * @param index A non-null pointer to an index that is populated if a request
   *              for this nanoapp is found.
   * @return A pointer to a SensorRequest that is owned by the provided nanoapp
   *         if one is found otherwise nullptr.
   */
  const SensorRequest *getSensorRequestForNanoapp(
      const SensorRequests& requests, const Nanoapp *nanoapp,
      size_t *index) const;
};

}  // namespace chre

#endif  // CHRE_CORE_SENSOR_REQUEST_MANAGER_H_
