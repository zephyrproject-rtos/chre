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

#include "chre/core/event_loop.h"

#include "chre/core/event.h"
#include "chre/core/event_loop_manager.h"
#include "chre/core/nanoapp.h"
#include "chre/platform/context.h"
#include "chre/platform/log.h"
#include "chre_api/chre/version.h"

namespace chre {

EventLoop::EventLoop()
    : mTimerPool(*this) {}

bool EventLoop::findNanoappInstanceIdByAppId(uint64_t appId,
                                             uint32_t *instanceId) {
  CHRE_ASSERT(instanceId != nullptr);

  // TODO: would be nice to have a ConditionalLockGuard where we just pass this
  // bool to the constructor and it automatically handles the unlock for us
  bool needLock = (getCurrentEventLoop() != this);
  if (needLock) {
    mNanoappsLock.lock();
  }

  bool found = false;
  for (auto& app : mNanoapps) {
    if (app->getAppId() == appId) {
      *instanceId = app->getInstanceId();
      found = true;
      break;
    }
  }

  if (needLock) {
    mNanoappsLock.unlock();
  }

  return found;
}

void EventLoop::forEachNanoapp(NanoappCallbackFunction *callback, void *data) {
  bool needLock = (getCurrentEventLoop() != this);
  if (needLock) {
    mNanoappsLock.lock();
  }

  for (const auto& nanoapp : mNanoapps) {
    callback(nanoapp.get(), data);
  }

  if (needLock) {
    mNanoappsLock.unlock();
  }
}

void EventLoop::run() {
  LOGI("EventLoop start");
  mRunning = true;

  bool havePendingEvents = false;
  while (mRunning) {
    // TODO: document the two-stage event delivery process further... general
    // idea is we block in mEvents.pop() if we know that no apps have pending
    // events
    if (!havePendingEvents || !mEvents.empty()) {
      // TODO: this is *not* thread-safe; if we have multiple EventLoops, then
      // there is no safety mechanism that ensures an event is not freed twice,
      // or that its free callback is invoked in the proper EventLoop, etc.
      Event *event = mEvents.pop();
      for (auto& app : mNanoapps) {
        if ((event->targetInstanceId == chre::kBroadcastInstanceId
                && app->isRegisteredForBroadcastEvent(event->eventType))
            || event->targetInstanceId == app->getInstanceId()) {
          app->postEvent(event);
        }
      }

      if (event->isUnreferenced()) {
        // Events sent to the system instance ID are processed via the free
        // callback and are not expected to be delivered to any nanoapp, so no
        // need to log a warning in that case
        if (event->senderInstanceId != kSystemInstanceId) {
          LOGW("Dropping event 0x%" PRIx16, event->eventType);
        }
        freeEvent(event);
      }
    }

    // TODO: most basic round-robin implementation - we might want to have some
    // kind of priority in the future, but this should be good enough for now
    havePendingEvents = false;
    for (auto& app : mNanoapps) {
      if (app->hasPendingEvent()) {
        // TODO: cleaner way to set/clear this? RAII-style?
        mCurrentApp = app.get();
        Event *event = app->processNextEvent();
        mCurrentApp = nullptr;

        if (event->isUnreferenced()) {
          freeEvent(event);
        }

        havePendingEvents |= app->hasPendingEvent();
      }
    }
  }

  while (!mNanoapps.empty()) {
    stopNanoapp(mNanoapps.front().get());
  }

  LOGI("Exiting EventLoop");

  // TODO: need to purge/cleanup events, etc.
}

bool EventLoop::startNanoapp(PlatformNanoapp *platformNanoapp) {
  CHRE_ASSERT(platformNanoapp != nullptr);

  bool success = false;
  if (!mNanoapps.prepareForPush()) {
    LOGE("Failed to allocate space for new nanoapp");
  } else {
    // TODO: get these parameters from somewhere
    UniquePtr<Nanoapp> nanoapp(
        0, 1, CHRE_API_VERSION_1_1,
        EventLoopManagerSingleton::get()->getNextInstanceId(), true,
        platformNanoapp);
    if (nanoapp.isNull()) {
      LOGE("Failed to allocate new nanoapp");
    } else {
      mCurrentApp = nanoapp.get();
      success = nanoapp->start();
      mCurrentApp = nullptr;
      if (!success) {
        LOGE("Nanoapp %" PRIu32 " failed to start", nanoapp->getInstanceId());
      } else {
        LockGuard<Mutex> lock(mNanoappsLock);
        mNanoapps.push_back(std::move(nanoapp));
      }
    }
  }

  return success;
}

void EventLoop::stopNanoapp(Nanoapp *nanoapp) {
  CHRE_ASSERT(nanoapp != nullptr);

  for (size_t i = 0; i < mNanoapps.size(); i++) {
    if (nanoapp == mNanoapps[i].get()) {
      {
        LockGuard<Mutex> lock(mNanoappsLock);
        mNanoapps.erase(i);
      }

      mCurrentApp = nanoapp;
      nanoapp->stop();
      mCurrentApp = nullptr;
      return;
    }
  }

  CHRE_ASSERT_LOG(false,
                  "Attempted to stop a nanoapp that is not already running");
}

bool EventLoop::postEvent(uint16_t eventType, void *eventData,
    chreEventCompleteFunction *freeCallback, uint32_t senderInstanceId,
    uint32_t targetInstanceId) {
  // TODO: Consider adding a CHRE_ASSERT(mRunning) here.
  bool success = false;
  Event *event = mEventPool.allocate(eventType, eventData, freeCallback,
      senderInstanceId, targetInstanceId);
  if (event != nullptr) {
    success = mEvents.push(event);
  } else {
    LOGE("Failed to allocate event");
  }
  return success;
}

void EventLoop::stop() {
  postEvent(0, nullptr, nullptr, kSystemInstanceId, kSystemInstanceId);
  mRunning = false;
}

Nanoapp *EventLoop::getCurrentNanoapp() const {
  CHRE_ASSERT(getCurrentEventLoop() == this);
  return mCurrentApp;
}

size_t EventLoop::getNanoappCount() const {
  CHRE_ASSERT(getCurrentEventLoop() == this);
  return mNanoapps.size();
}

TimerPool& EventLoop::getTimerPool() {
  return mTimerPool;
}

Nanoapp *EventLoop::findNanoappByInstanceId(uint32_t instanceId) {
  bool needLock = (getCurrentEventLoop() != this);
  if (needLock) {
    mNanoappsLock.lock();
  }

  Nanoapp *nanoapp = lookupAppByInstanceId(instanceId);

  if (needLock) {
    mNanoappsLock.unlock();
  }

  return nanoapp;
}

Nanoapp *EventLoop::lookupAppByInstanceId(uint32_t instanceId) {
  // The system instance ID always has nullptr as its Nanoapp pointer, so can
  // skip iterating through the nanoapp list for that case
  if (instanceId != kSystemInstanceId) {
    for (auto& app : mNanoapps) {
      if (app->getInstanceId() == instanceId) {
        return app.get();
      }
    }
  }

  return nullptr;
}

void EventLoop::freeEvent(Event *event) {
  if (event->freeCallback != nullptr) {
    // TODO: find a better way to set the context to the creator of the event
    mCurrentApp = lookupAppByInstanceId(event->senderInstanceId);
    event->freeCallback(event->eventType, event->eventData);
    mCurrentApp = nullptr;

    mEventPool.deallocate(event);
  }
}

}  // namespace chre
