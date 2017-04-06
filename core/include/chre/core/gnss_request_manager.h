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

#include "chre/core/nanoapp.h"
#include "chre/platform/platform_gnss.h"
#include "chre/util/non_copyable.h"
#include "chre/util/time.h"

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
   * Initializes a GnssRequestManager.
   */
  GnssRequestManager();

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
   * Tracks a nanoapp that has subscribed to a location session and the
   * reporting interval.
   */
  struct LocationSessionRequest {
    //! The nanoapp instance ID that made this request.
    uint32_t nanoappInstanceId;

    //! The interval of location results requested.
    Milliseconds minInterval;
  };

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
  DynamicVector<LocationSessionRequest> mLocationSessionRequests;

  //! The current interval being sent to the location session. This is only
  //! valid if the mLocationSessionRequests is non-empty.
  Milliseconds mCurrentLocationSessionInterval;

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

  /**
   * Checks if a nanoapp has an open location session request.
   *
   * @param instanceId The nanoapp instance ID to search for.
   * @param requestIndex A pointer to an index to populate if the nanoapp has an
   *        open location session.
   * @return true if the provided instanceId was found.
   */
  bool nanoappHasLocationSessionRequest(uint32_t instanceId,
                                        size_t *requestIndex = nullptr);

  /**
   * Adds a request for a location session to the queue of state transitions.
   *
   * @param instanceId The nanoapp instance ID requesting a location session.
   * @param enable Whether the location session is being enabled or disabled for
   *        this nanoapp.
   * @param minInterval The minimum interval reqested by the nanoapp.
   * @param cookie A cookie that is round-tripped to the nanoapp for context.
   * @return true if the state transition was added to the queue.
   */
  bool addLocationSessionRequestToQueue(uint32_t instanceId, bool enable,
                                        Milliseconds minInterval,
                                        const void *cookie);

  /**
   * @return true if the location session is currently enabled.
   */
  bool locationSessionIsEnabled();

  /**
   * Determines if the location session is already in the requested state.
   *
   * @param requestedState The target state of the location session.
   * @param minInterval The reporting interval if the requestedState is true.
   * @param nanoappHasRequest true if the requesting nanoapp already has an
   *        outstanding request.
   * @return true if the location session is already in the requested state.
   */
  bool locationSessionIsInRequestedState(bool requestedState,
                                         Milliseconds minInterval,
                                         bool nanoappHasRequest);

  /**
   * Determines if a change to the location session state is required given a
   * set of parameters.
   *
   * @param requestedState The target state requested by a nanoapp.
   * @param minInterval The minimum location reporting interval.
   * @param nanoappHasRequest If the nanoapp already has a request.
   * @param requestIndex The index of the request in the list of open requests
   *        if nanoappHasRequest is set to true.
   * @return true if a state transition is required.
   */
  bool locationSessionStateTransitionIsRequired(bool requestedState,
                                                Milliseconds minInterval,
                                                bool nanoappHasRequest,
                                                size_t requestIndex);

  /**
   * Updates the location session requests given a nanoapp and the interval
   * requested.
   *
   * @param enable true if enabling the location session.
   * @param minInterval the minimum reporting interval if enable is set to true.
   * @param instanceId the nanoapp instance ID that owns the request.
   * @return true if the location session request list was updated.
   */
  bool updateLocationSessionRequests(bool enable, Milliseconds minInterval,
                                     uint32_t instanceId);

  /**
   * Posts the result of a GNSS location session start/stop request.
   *
   * @param instanceId The nanoapp instance ID that made the request.
   * @param success true if the operation was successful.
   * @param enable true if enabling the location session.
   * @param minInterval the minimum location reporting interval.
   * @param errorCode the error code as a result of this operation.
   * @param cookie the cookie that the nanoapp is provided for context.
   * @return true if the event was successfully posted.
   */
  bool postLocationSessionAsyncResultEvent(uint32_t instanceId, bool success,
                                           bool enable, Milliseconds minInterval,
                                           uint8_t errorCode, const void *cookie);

  /**
   * Releases the memory associated with an async GNSS result event.
   *
   * @param eventType The type of event to release.
   * @param eventData The event pointer to free.
   */
  static void freeGnssAsyncResultCallback(uint16_t eventType, void *eventData);
};

}  // namespace chre

#endif  // CHRE_CORE_GNSS_REQUEST_MANAGER_H_
