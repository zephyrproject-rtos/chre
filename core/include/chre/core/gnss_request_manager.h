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

#ifndef CHRE_CORE_GNSS_REQUEST_MANAGER_H_
#define CHRE_CORE_GNSS_REQUEST_MANAGER_H_

#include <cstdint>

#include "chre/core/gnss_request.h"
#include "chre/core/nanoapp.h"
#include "chre/core/request_multiplexer.h"
#include "chre/platform/platform_gnss.h"
#include "chre/util/non_copyable.h"

namespace chre {

/**
 * The GnssRequestManager handles requests from nanoapps for GNSS data. This
 * includes multiplexing multiple requests into one for the platform to handle.
 *
 * This class is effectively a singleton as there can only be one instance of
 * the PlatformGnss instance.
 */
class GnssRequestManager : public NonCopyable {
 public:
  /**
   * @return the GNSS capabilities exposed by this platform.
   */
  uint32_t getCapabilities();

  /**
   * Starts a location session asynchronously. The result is delivered through
   * a CHRE_EVENT_GNSS_ASYNC_RESULT event.
   *
   * @param nanoapp The nanoapp requesting the location session.
   * @param minInterval The minimum reporting interval for location results.
   * @param timeToNextFix The amount of time that the locationing system is
   *        allowed to delay generating a fix.
   * @param cookie A cookie that is round-tripped to provide context to the
   *               nanoapp making the request.
   * @return true if the request was accepted for processing.
   */
  bool startLocationSession(Nanoapp *nanoapp, Milliseconds minInterval,
                            Milliseconds minTimeToNextFix, const void *cookie);

  /**
   * Stops a location session asynchronously. The result is delivered through a
   * CHRE_EVENT_GNSS_ASYNC_RESULT event.
   *
   * @param nanoapp The nanoapp requesting the location session to stop.
   * @param cookie A cookie that is round-tripped to provide context to the
   *        nanoapp making the request.
   *  @return true if the request was accepted for processing.
   */
  bool stopLocationSession(Nanoapp *nanoapp, const void *cookie);

 private:
  /**
   * Tracks the state of the locationing engine.
   */
  struct LocationSessionStateTransition {
    //! The nanoapp instance ID that prompted the change.
    uint32_t nanoappInstanceId;

    //! The cookie provided to the CHRE API when the nanoapp requested a change
    //! to the state of the location engine.
    const void *cookie;

    //! The target state of the location engine.
    bool enable;

    //! The target minimum reporting interval for the location engine. This is
    //! only valid if enable is set to true.
    Milliseconds minInterval;

    //! The timestamp of the time this request was made.
    Nanoseconds stateTransitionStartTime;

    //! The minimum time to the first fix.
    Milliseconds minTimeToFirstFix;
  };

  //! The maximum number of state transitions allowed for location and GNSS
  //! measurement resources.
  static constexpr size_t kMaxGnssStateTransitions = 8;

  //! The instance of the platform GNSS interface.
  PlatformGnss mPlatformGnss;

  //! The queue of state transitions for the location engine. Only one
  //! asynchronous location engine state transition can be in flight at one
  //! time. Any further requests are queued here.
  ArrayQueue<LocationSessionStateTransition,
             kMaxGnssStateTransitions> mLocationSessionStateTransitions;

  //! The request multiplexer for GNSS location requests.
  RequestMultiplexer<GnssRequest> mLocationRequestManager;

  /**
   * Configures the location engine to be enabled/disabled. If enable is set to
   * true then the minInterval and minTimeToFirstFix values are valid.
   *
   * @param nanoapp The nanoapp requesting the state change for the location
   *        engine.
   * @param enable Whether to enable or disable the location engine.
   * @param minInterval The minimum location reporting interval requested by the
   *        nanoapp.
   * @param minTimeToFirstFix The minimum time to the first fix.
   * @param cookie The cookie provided by the nanoapp to round-trip for context.
   * @return true if the request was accepted.
   */
  bool configureLocationSession(Nanoapp *nanoapp, bool enable,
                                Milliseconds minInterval,
                                Milliseconds minTimeToFirstFix,
                                const void *cookie);
};

}  // namespace chre

#endif  // CHRE_CORE_GNSS_REQUEST_MANAGER_H_
