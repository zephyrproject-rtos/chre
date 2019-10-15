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

#ifndef CHRE_CORE_SENSOR_REQUESTS_H_
#define CHRE_CORE_SENSOR_REQUESTS_H_

#include "chre/core/request_multiplexer.h"
#include "chre/core/sensor.h"
#include "chre/core/sensor_request.h"
#include "chre/core/timer_pool.h"
#include "chre/platform/atomic.h"

namespace chre {

/**
 * Keeps track of the state of a sensor along with the various requests for it.
 */
// TODO(b/139693714): Inherit from RequestMultiplexer to simplify adding
// additional functionality on top of the multiplexer.
class SensorRequests {
 public:
  SensorRequests() : mFlushRequestPending(false) {}

  /**
   * Initializes the sensor object. This method must only be invoked once
   * when the SensorRequestManager initializes.
   *
   * @param sensor The sensor object to initialize with.
   */
  void setSensor(const Sensor &&sensor) {
    mSensor = std::move(sensor);
  }

  /**
   * @return The set of active requests for this sensor.
   */
  const DynamicVector<SensorRequest> &getRequests() const {
    return mMultiplexer.getRequests();
  }

  /**
   * @return true if the sensor is currently enabled.
   */
  bool isSensorEnabled() const {
    return !mMultiplexer.getRequests().empty();
  }

  /**
   * @return A constant reference to the sensor object. This method has an
   * undefined behavior if isSensorSupported() is false.
   */
  const Sensor &getSensor() const {
    return mSensor;
  }

  /**
   * @return A reference to the sensor object. This method has an undefined
   * behavior if isSensorSupported() is false.
   */
  Sensor &getSensor() {
    return mSensor;
  }

  /**
   * @return A reference to the request multiplexer used to store all active
   * requests for this sensor.
   */
  RequestMultiplexer<SensorRequest> &getMultiplexer() {
    return mMultiplexer;
  }

  /**
   * Clears any states (e.g. timeout timer and relevant flags) associated
   * with a pending flush request.
   */
  void clearPendingFlushRequest();

  /**
   * Cancels the pending timeout timer associated with a flush request.
   */
  void cancelPendingFlushRequestTimer();

  /**
   * Sets the timer handle used to time out an active flush request for this
   * sensor.
   *
   * @param handle Timer handle for the current flush request.
   */
  void setFlushRequestTimerHandle(TimerHandle handle) {
    mFlushRequestTimerHandle = handle;
  }

  /**
   * Sets whether a flush request is pending or not.
   *
   * @param pending bool indicating whether a flush is pending.
   */
  void setFlushRequestPending(bool pending) {
    mFlushRequestPending = pending;
  }

  /**
   * @return true if a flush is pending.
   */
  inline bool isFlushRequestPending() const {
    return mFlushRequestPending;
  }

 private:
  // TODO(b/139693714): Make SensorRequests a member of Sensor to make the
  // relationship between the two more explicit since it's odd to have requests
  // owning a sensor.
  //! The sensor associated with this request multiplexer.
  Sensor mSensor;

  //! The request multiplexer for this sensor.
  RequestMultiplexer<SensorRequest> mMultiplexer;

  //! The timeout timer handle for the current flush request.
  TimerHandle mFlushRequestTimerHandle = CHRE_TIMER_INVALID;

  //! True if a flush request is pending for this sensor.
  AtomicBool mFlushRequestPending;
};

}  // namespace chre

#endif  // CHRE_CORE_SENSOR_REQUESTS_H_