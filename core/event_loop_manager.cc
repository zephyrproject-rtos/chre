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

Nanoapp *EventLoopManager::validateChreApiCall(const char *functionName,
                                               EventLoop **outputEventLoop) {
  chre::EventLoop *eventLoop = getCurrentEventLoop();
  CHRE_ASSERT(eventLoop);
  if (outputEventLoop != nullptr) {
    *outputEventLoop = eventLoop;
  }

  chre::Nanoapp *currentNanoapp = eventLoop->getCurrentNanoapp();
  CHRE_ASSERT_LOG(currentNanoapp, "%s called with no CHRE app context", __func__);

  return currentNanoapp;
}

EventLoop *EventLoopManager::createEventLoop() {
  mEventLoops.emplace_back();
  return &mEventLoops.back();
}

bool EventLoopManager::deferCallback(SystemCallbackType type, void *data,
                                     SystemCallbackFunction *callback) {
  // TODO: when multiple EventLoops are supported, consider allowing the
  // platform to define which EventLoop is used to process system callbacks.
  CHRE_ASSERT(!mEventLoops.empty());
  return mEventLoops[0].postEvent(static_cast<uint16_t>(type), data, callback,
                                  kSystemInstanceId, kSystemInstanceId);
}

bool EventLoopManager::findNanoappInstanceIdByAppId(
    uint64_t appId, uint32_t *instanceId, EventLoop **eventLoopOut) {
  bool found = false;

  for (EventLoop& eventLoop : mEventLoops) {
    if (eventLoop.findNanoappInstanceIdByAppId(appId, instanceId)) {
      found = true;
      if (eventLoopOut != nullptr) {
        *eventLoopOut = &eventLoop;
      }
      break;
    }
  }

  return found;
}

Nanoapp *EventLoopManager::findNanoappByInstanceId(
    uint32_t instanceId, EventLoop **eventLoopOut) {
  Nanoapp *nanoapp = nullptr;
  for (EventLoop& eventLoop : mEventLoops) {
    nanoapp = eventLoop.findNanoappByInstanceId(instanceId);
    if (nanoapp != nullptr) {
      if (eventLoopOut != nullptr) {
        *eventLoopOut = &eventLoop;
      }
      break;
    }
  }

  return nanoapp;
}

uint32_t EventLoopManager::getNextInstanceId() {
  // TODO: this needs to be an atomic integer when we have > 1 event loop, or
  // use a mutex
  static_assert(kNumEventLoops == 1, "Need to make this atomic");
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

bool EventLoopManager::populateNanoappInfoForAppId(
    uint64_t appId, struct chreNanoappInfo *info) const {
  bool success = false;
  for (const EventLoop& eventLoop : mEventLoops) {
    success = eventLoop.populateNanoappInfoForAppId(appId, info);
    if (success) {
      break;
    }
  }

  return success;
}

bool EventLoopManager::populateNanoappInfoForInstanceId(
    uint32_t instanceId, struct chreNanoappInfo *info) const {
  bool success = false;
  for (const EventLoop& eventLoop : mEventLoops) {
    success = eventLoop.populateNanoappInfoForInstanceId(instanceId, info);
    if (success) {
      break;
    }
  }

  return success;
}

bool EventLoopManager::postEvent(uint16_t eventType, void *eventData,
                                 chreEventCompleteFunction *freeCallback,
                                 uint32_t senderInstanceId,
                                 uint32_t targetInstanceId) {
  LockGuard<Mutex> lock(mMutex);

  // TODO: for unicast events, ideally we'd just post the event to the EventLoop
  // that has the target
  bool success = true;
  for (EventLoop& eventLoop : mEventLoops) {
    success &= eventLoop.postEvent(eventType, eventData, freeCallback,
                                   senderInstanceId, targetInstanceId);
  }

  return success;
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
