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

#include "chre/core/nanoapp.h"

#include "chre/platform/assert.h"
#include "chre/platform/fatal_error.h"
#include "chre/platform/log.h"

namespace chre {

Nanoapp::Nanoapp(uint32_t instanceId, PlatformNanoapp *platformNanoapp)
    : mInstanceId(instanceId), mPlatformNanoapp(platformNanoapp) {}

bool Nanoapp::isRegisteredForBroadcastEvent(uint16_t eventType) const {
  return (mRegisteredEvents.find(eventType) != mRegisteredEvents.size());
}

bool Nanoapp::registerForBroadcastEvent(uint16_t eventId) {
  if (isRegisteredForBroadcastEvent(eventId)) {
    return false;
  }

  if (!mRegisteredEvents.push_back(eventId)) {
    FATAL_ERROR("App failed to register for event. Out of memory.");
  }

  return true;
}

uint64_t Nanoapp::getAppId() const {
  return mAppId;
}

uint32_t Nanoapp::getInstanceId() const {
  return mInstanceId;
}

void Nanoapp::postEvent(Event *event) {
  mEventQueue.push(event);
}

bool Nanoapp::start() {
  return mPlatformNanoapp->start();
}

void Nanoapp::stop() {
  mPlatformNanoapp->stop();
}

bool Nanoapp::hasPendingEvent() {
  return !mEventQueue.empty();
}

Event *Nanoapp::processNextEvent() {
  Event *event = mEventQueue.pop();

  if (event != nullptr) {
    LOGD("Delivering event %" PRIu16 " to instance %" PRIu32,
         event->eventType, mInstanceId);
    mPlatformNanoapp->handleEvent(event->senderInstanceId, event->eventType,
                             event->eventData);
  } else {
    CHRE_ASSERT_LOG(false, "Tried delivering event, but queue empty");
  }

  return event;
}

}  // namespace chre
