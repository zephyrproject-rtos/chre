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

#include "chre/core/event_loop_manager.h"
#include "chre/platform/assert.h"
#include "chre/platform/fatal_error.h"
#include "chre/platform/log.h"
#include "chre/util/system/debug_dump.h"

namespace chre {

Nanoapp::Nanoapp() {
  // Push first bucket onto wakeup bucket queue
  cycleWakeupBuckets(1);
}

Nanoapp::~Nanoapp() {
  CHRE_ASSERT_LOG(getTotalAllocatedBytes() == 0,
      "Nanoapp ID=0x%016" PRIx64 " still has %zu allocated bytes!", getAppId(),
      getTotalAllocatedBytes());
}

bool Nanoapp::isRegisteredForBroadcastEvent(uint16_t eventType) const {
  return (mRegisteredEvents.find(eventType) != mRegisteredEvents.size());
}

bool Nanoapp::registerForBroadcastEvent(uint16_t eventId) {
  if (isRegisteredForBroadcastEvent(eventId)) {
    return false;
  }

  if (!mRegisteredEvents.push_back(eventId)) {
    FATAL_ERROR_OOM();
  }

  return true;
}

bool Nanoapp::unregisterForBroadcastEvent(uint16_t eventId) {
  size_t registeredEventIndex = mRegisteredEvents.find(eventId);
  if (registeredEventIndex == mRegisteredEvents.size()) {
    return false;
  }

  mRegisteredEvents.erase(registeredEventIndex);
  return true;
}

void Nanoapp::configureNanoappInfoEvents(bool enable) {
  if (enable) {
    registerForBroadcastEvent(CHRE_EVENT_NANOAPP_STARTED);
    registerForBroadcastEvent(CHRE_EVENT_NANOAPP_STOPPED);
  } else {
    unregisterForBroadcastEvent(CHRE_EVENT_NANOAPP_STARTED);
    unregisterForBroadcastEvent(CHRE_EVENT_NANOAPP_STOPPED);
  }
}

void Nanoapp::configureHostSleepEvents(bool enable) {
  if (enable) {
    registerForBroadcastEvent(CHRE_EVENT_HOST_AWAKE);
    registerForBroadcastEvent(CHRE_EVENT_HOST_ASLEEP);
  } else {
    unregisterForBroadcastEvent(CHRE_EVENT_HOST_AWAKE);
    unregisterForBroadcastEvent(CHRE_EVENT_HOST_ASLEEP);
  }
}

Event *Nanoapp::processNextEvent() {
  Event *event = mEventQueue.pop();

  CHRE_ASSERT_LOG(event != nullptr, "Tried delivering event, but queue empty");
  if (event != nullptr) {
    handleEvent(event->senderInstanceId, event->eventType, event->eventData);
  }

  return event;
}

void Nanoapp::blameHostWakeup() {
  LOGI("Recorded host wakeup for nanoapp Id=%" PRIu32, getInstanceId());
  if (mWakeupBuckets.back() < UINT8_MAX) ++mWakeupBuckets.back();
}

void Nanoapp::cycleWakeupBuckets(size_t numBuckets) {
  numBuckets = numBuckets > kMaxSizeWakeupBuckets ?
      kMaxSizeWakeupBuckets : numBuckets;
  for (size_t i = 0; i < numBuckets; ++i) {
    mWakeupBuckets.kick_push(0);
  }
}

void Nanoapp::logStateToBuffer(char *buffer, size_t *bufferPos,
                               size_t bufferSize) const {
  PlatformNanoapp::logStateToBuffer(buffer, bufferPos, bufferSize);
  debugDumpPrint(
      buffer, bufferPos, bufferSize,
      " Id=%" PRIu32 " AppId=0x%016" PRIx64
      " ver=0x%" PRIx32 " targetAPI=0x%" PRIx32
      " currentAllocatedBytes=%zu peakAllocatedBytes=%zu",
      getInstanceId(), getAppId(), getAppVersion(), getTargetApiVersion(),
      getTotalAllocatedBytes(), getPeakAllocatedBytes());
  logWakeupsStateToBuffer(buffer, bufferPos, bufferSize);
}

void Nanoapp::logWakeupsStateToBuffer(char *buffer, size_t *bufferPos,
                                      size_t bufferSize) const {
  debugDumpPrint(buffer, bufferPos, bufferSize, " HostWakeups=[ Latest-> ");
  // Get buckets latest -> earliest except last one
  for (size_t i = mWakeupBuckets.size() - 1; i > 0; --i) {
    debugDumpPrint(buffer, bufferPos, bufferSize,
                   "%" PRIu8 ", ", mWakeupBuckets[i]);
  }
  // earliest bucket gets no comma
  debugDumpPrint(buffer, bufferPos, bufferSize, "%" PRIu8 " ]\n",
                 mWakeupBuckets.back());
}

}  // namespace chre
