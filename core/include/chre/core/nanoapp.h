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

#ifndef CHRE_CORE_NANOAPP_H_
#define CHRE_CORE_NANOAPP_H_

#include <cinttypes>

#include "chre/core/event.h"
#include "chre/core/event_ref_queue.h"
#include "chre/platform/platform_nanoapp.h"
#include "chre/util/dynamic_vector.h"
#include "chre/util/non_copyable.h"

namespace chre {

/**
 * A class that tracks the state of a Nanoapp including incoming events and
 * event registrations.
 */
class Nanoapp : public NonCopyable {
 public:
  /**
   * Constructs a Nanoapp that manages the lifecycle of events and calls into
   * the entrypoints of the app.
   *
   * @param A unique instance ID for this nanoapp.
   * @param A pointer to the platform-specific nanoapp functionality which
   *        allows calling the entry points and managing the lifecycle of the
   *        app (such as unloading the app).
   */
  Nanoapp(uint32_t instanceId, PlatformNanoapp *platformNanoapp);

  uint64_t getAppId() const;
  uint32_t getInstanceId() const;

  bool isRegisteredForBroadcastEvent(uint16_t eventType) const;

  /**
   * Updates the Nanoapp's registration so that it will receive broadcast events
   * with the given event ID.
   *
   * @return true if the event is newly registered
   */
  bool registerForBroadcastEvent(uint16_t eventId);

  /**
   * Updates the Nanoapp's registration so that it will not receive broadcast
   * events with the given event ID.
   *
   * @return true if the event was previously registered
   */
  bool unregisterForBroadcastEvent(uint16_t eventId);

  void postEvent(Event *event);

  /**
   * Starts the nanoapp by invoking the start handler and returns the result of
   * the handler.
   *
   * @return True indicating that the app was started successfully.
   */
  bool start();

  /**
   * Stops the nanoapp by invoking the stop handler.
   */
  void stop();

  /**
   * Indicates whether there are any pending events in this apps queue.
   *
   * @return True indicating that there are events available to be processed.
   */
  bool hasPendingEvent();

  /**
   * Sends the next event in the queue to the nanoapp and returns the processed
   * event. The hasPendingEvent() method should be tested before invoking this.
   *
   * @return a pointer to the processed event.
   */
  Event *processNextEvent();

 private:
  const uint64_t mAppId = 0;
  const uint32_t mInstanceId;
  PlatformNanoapp * const mPlatformNanoapp;

  //! The set of broadcast events that this app is registered for.
  // TODO: Implement a set container and replace DynamicVector here. There may
  // also be a better way of handling this (perhaps we map event type to apps
  // who care about them).
  DynamicVector<uint16_t> mRegisteredEvents;

  EventRefQueue mEventQueue;
};

}

#endif  // CHRE_CORE_NANOAPP_H_
