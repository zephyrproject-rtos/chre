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

#ifndef CHRE_PLATFORM_SLPI_SEE_PLATFORM_SENSOR_BASE_H_
#define CHRE_PLATFORM_SLPI_SEE_PLATFORM_SENSOR_BASE_H_

extern "C" {

#include "sns_std_type.pb.h"  // for sns_std_suid

}  // extern "C"

#include "chre/core/sensor_request.h"

namespace chre {

//! The max length of sensorName
constexpr size_t kSensorNameMaxLen = 64;

/**
 * Storage for the SLPI SEE implementation of the PlatformSensor class.
 */
class PlatformSensorBase {
 public:
  /**
   * Initializes the members of PlatformSensorBase.
   */
  void initBase(const sns_std_suid& suid, SensorType sensorType,
                bool calibrated, uint64_t mMinInterval, const char *sensorName,
                ChreSensorData *lastEvent, size_t lastEventSize);

  /**
   * Copies the supplied event to the sensor's last event and marks last event
   * valid.
   *
   * @param event The pointer to the event to copy from.
   */
  void setLastEvent(const ChreSensorData *event);

 protected:
  //! The SUID of this sensor
  sns_std_suid mSuid;

  //! The sensor type of this sensor.
  SensorType mSensorType;

  //! Whether the sensor is runtime-calibrated if applicable.
  bool mCalibrated;

  //! The minimum interval of this sensor.
  uint64_t mMinInterval;

  //! The name (type and model) of this sensor.
  char mSensorName[kSensorNameMaxLen];

  //! Pointer to dynamically allocated memory to store the last event. Only
  //! non-null if this is an on-change sensor.
  ChreSensorData *mLastEvent = nullptr;

  //! The amount of memory we've allocated in lastEvent (this varies depending
  //! on the sensor type)
  size_t mLastEventSize = 0;

  //! Set to true only when this is an on-change sensor that is currently active
  //! and we have a copy of the most recent event in lastEvent.
  bool mLastEventValid = false;

  //! Whether the sensor is turned off. This can be different from what's been
  //! requested through Sensor::setRequest() as a passive request may not
  //! always be honored by PlatformSensor and the sensor can stay off.
  bool mIsSensorOff = true;

  //! Stores the sampling status for all CHRE clients of this sensor.
  struct chreSensorSamplingStatus mSamplingStatus;
};

}  // namespace chre

#endif  // CHRE_PLATFORM_SLPI_SEE_PLATFORM_SENSOR_BASE_H_
