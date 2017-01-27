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

#include "chre/util/lock_guard.h"

namespace chre {

EventLoop *EventLoopManager::createEventLoop() {
  if (!mEventLoops.emplace_back()) {
    return nullptr;
  }

  return mEventLoops.back().get();
}

void EventLoopManager::postEvent(uint16_t eventType, void *eventData,
                                 chreEventCompleteFunction *freeCallback,
                                 uint32_t senderInstanceId,
                                 uint32_t targetInstanceId) {
  LockGuard<Mutex> lock(mMutex);
  for (size_t i = 0; i < mEventLoops.size(); i++) {
    // TODO: Check to see if anyone in the event loop cares about this event and
    // consider not posting it.
    mEventLoops[i]->postEvent(eventType, eventData, freeCallback,
                              senderInstanceId, targetInstanceId);
  }
}

SensorRequestManager& EventLoopManager::getSensorRequestManager() {
  return mSensorRequestManager;
}

WifiRequestManager& EventLoopManager::getWifiRequestManager() {
  return mWifiRequestManager;
}

}  // namespace chre
