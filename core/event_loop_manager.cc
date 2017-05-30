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

#include "chre/core/event_loop_manager.h"

#include "chre/platform/context.h"
#include "chre/platform/fatal_error.h"
#include "chre/platform/memory.h"
#include "chre/util/lock_guard.h"

namespace chre {

void freeEventDataCallback(uint16_t /*eventType*/, void *eventData) {
  memoryFree(eventData);
}

Nanoapp *EventLoopManager::validateChreApiCall(const char *functionName) {
  chre::Nanoapp *currentNanoapp = EventLoopManagerSingleton::get()
      ->getEventLoop().getCurrentNanoapp();
  CHRE_ASSERT_LOG(currentNanoapp, "%s called with no CHRE app context",
                  functionName);
  return currentNanoapp;
}

bool EventLoopManager::deferCallback(SystemCallbackType type, void *data,
                                     SystemCallbackFunction *callback) {
  return mEventLoop.postEvent(static_cast<uint16_t>(type), data, callback,
                              kSystemInstanceId, kSystemInstanceId);
}

uint32_t EventLoopManager::getNextInstanceId() {
  ++mLastInstanceId;

  // ~4 billion instance IDs should be enough for anyone... if we need to
  // support wraparound for stress testing load/unload, then we can set a flag
  // when wraparound occurs and use EventLoop::findNanoappByInstanceId to ensure
  // we avoid conflicts
  if (mLastInstanceId == kBroadcastInstanceId
      || mLastInstanceId == kSystemInstanceId) {
    FATAL_ERROR("Exhausted instance IDs!");
  }

  return mLastInstanceId;
}

EventLoop& EventLoopManager::getEventLoop() {
  return mEventLoop;
}

GnssRequestManager& EventLoopManager::getGnssRequestManager() {
  return mGnssRequestManager;
}

HostCommsManager& EventLoopManager::getHostCommsManager() {
  return mHostCommsManager;
}

SensorRequestManager& EventLoopManager::getSensorRequestManager() {
  return mSensorRequestManager;
}

WifiRequestManager& EventLoopManager::getWifiRequestManager() {
  return mWifiRequestManager;
}

WwanRequestManager& EventLoopManager::getWwanRequestManager() {
  return mWwanRequestManager;
}

MemoryManager& EventLoopManager::getMemoryManager() {
  return mMemoryManager;
}

}  // namespace chre
