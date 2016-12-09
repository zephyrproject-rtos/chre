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

#ifndef CHRE_CORE_TIMER_POOL_H_
#define CHRE_CORE_TIMER_POOL_H_

// TODO: common timer module
//  - provide callback interface, build delayed event capability on top
//  - eventually, condense to single system timer (i.e. one that fires next),
//    but for now, can map 1:1 into system timer
//  - collection of pending timer events (list initially, but priority queue would be nice)

#include "chre/platform/system_timer.h"
#include "chre/util/dynamic_vector.h"

namespace chre {

/**
 * The type to use when referring to a timer instance.
 *
 * Note that this mirrors the CHRE API definition of a timer handle, so should
 * not be changed without appropriate consideration.
 */
typedef uint32_t TimerHandle;

//! Forward declare the EventLoop class to avoid a circular dependency between
//! TimerPool and EventLoop.
class EventLoop;

/**
 * Tracks requests from CHRE apps for timed events.
 */
class TimerPool {
 public:
  /**
   * Sets up the timer instance initial conditions.
   *
   * @param The event loop that owns this timer.
   */
  TimerPool(EventLoop& eventLoop);

  /**
   * Requests a timer for the currently running nanoapp given a cookie to pass
   * to the nanoapp when the timer event is published.
   *
   * @param The duration of the timer.
   * @param A cookie to pass to the app when the timer elapses.
   * @param Whether or not the timer should automatically re-arm.
   */
  TimerHandle setTimer(Nanoseconds duration, void *cookie, bool oneShot);

  // TODO: should also add methods here to:
  //   - post an event after a delay
  //   - invoke a callback in "unsafe" context (i.e. from other thread), which the
  //     CHRE system could use to trigger things while the task runner is busy

 private:
  /**
   * Tracks metadata associated with a request for a timed event.
   */
  struct TimerRequest {
    //! The nanoapp from which this request was made.
    Nanoapp *requestingNanoapp;

    //! The TimerHandle assigned to this request.
    TimerHandle timerHandle;

    //! The time when the request was made.
    Nanoseconds expirationTime;

    //! The requested duration of the timer.
    Nanoseconds duration;

    //! Whether or not the request is a one shot or should be rescheduled.
    bool isOneShot;

    //! The cookie pointer to be passed as an event to the requesting nanoapp.
    void *cookie;
  };

  //! The event loop that owns this timer pool.
  EventLoop& mEventLoop;

  //! The list of outstanding timer requests.
  //TODO: Make this a priority queue.
  DynamicVector<TimerRequest> mTimerRequests;

  //! The underlying system timer used to schedule delayed callbacks.
  SystemTimer mSystemTimer;
};

}  // namespace chre

#endif  // CHRE_CORE_TIMER_POOL_H_
