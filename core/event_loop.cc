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
#include "chre/core/nanoapp.h"
#include "chre/platform/log.h"

namespace chre {

EventLoop::EventLoop()
    : mTimerPool(*this) {}

void EventLoop::run() {
  LOGI("EventLoop start");
  mRunning = true;

  bool havePendingEvents = false;
  while (mRunning) {
    // TODO: document the two-stage event delivery process further... general
    // idea is we block in mEvents.pop() if we know that no apps have pending
    // events
    if (!havePendingEvents || !mEvents.empty()) {
      Event *event = mEvents.pop();
      for (Nanoapp *app : mNanoapps) {
        if ((event->targetInstanceId == chre::kBroadcastInstanceId
                && app->isRegisteredForBroadcastEvent(event->eventType))
              || event->targetInstanceId == app->getInstanceId()) {
          app->postEvent(event);
        }
      }

      if (event->isUnreferenced()) {
        LOGW("Dropping event 0x%" PRIx16, event->eventType);
        freeEvent(event);
      }
    }

    // TODO: most basic round-robin implementation - we might want to have some
    // kind of priority in the future, but this should be good enough for now
    havePendingEvents = false;
    for (Nanoapp *app : mNanoapps) {
      if (app->hasPendingEvent()) {
        mCurrentApp = app;  // TODO: cleaner way to set/clear this? RAII-style?
        Event *event = app->processNextEvent();
        mCurrentApp = nullptr;

        if (event->isUnreferenced()) {
          freeEvent(event);
        }

        havePendingEvents |= app->hasPendingEvent();
      }
    }
  }

  LOGI("Exiting EventLoop");

  // TODO: need to purge/cleanup events, call task stop, etc.

}

bool EventLoop::startNanoapp(Nanoapp *nanoapp) {
  ASSERT(nanoapp != nullptr);

  // TODO: Test that the nanoapp list has space for one more nanoapp before
  // starting it. Return false if there is no space.

  mCurrentApp = nanoapp;
  bool nanoappIsStarted = nanoapp->start();
  mCurrentApp = nullptr;

  if (!nanoappIsStarted) {
    return false;
  }

  mNanoapps.push_back(nanoapp);
  return true;
}

void EventLoop::stopNanoapp(Nanoapp *nanoapp) {
  ASSERT(nanoapp != nullptr);

  for (size_t i = 0; i < mNanoapps.size(); i++) {
    if (nanoapp == mNanoapps[i]) {
      mNanoapps.erase(mNanoapps.begin() + i);

      mCurrentApp = nanoapp;
      nanoapp->stop();
      mCurrentApp = nullptr;
      return;
    }
  }

  ASSERT_LOG(false, "Attempted to stop a nanoapp that is not already running");
}

void EventLoop::postEvent(uint16_t eventType, void *eventData,
    chreEventCompleteFunction *freeCallback, uint32_t senderInstanceId,
    uint32_t targetInstanceId) {
  Event *event = mEventPool.allocate(eventType, eventData, freeCallback,
      senderInstanceId, targetInstanceId);
  if (event != nullptr) {
    mEvents.push(event);
  } else {
    LOGE("Failed to allocate event");
  }
}

void EventLoop::postEventDelayed(Event *event, uint64_t delayNs) {
  // TODO: use timer
  mEvents.push(event);
}

void EventLoop::stop() {
  mRunning = false;

  // TODO: provide a better interface that lets us unblock the event loop so
  // it notices that we want it to stop
  postEvent(0, nullptr, nullptr, kSystemInstanceId, kSystemInstanceId);
}

const Nanoapp *EventLoop::getCurrentNanoapp() const {
  return mCurrentApp;
}

uint32_t EventLoop::getNextInstanceId() {
  // This is a simple unique ID generator that checks a newly generated ID
  // against all existing IDs using a search (currently linear search).
  // Instance ID generation will slow with more apps. We generally expect there
  // to be few apps and IDs are generated infrequently.
  //
  // The benefit of generating IDs this way is that there is no memory overhead
  // to track issued IDs.
  uint32_t nextInstanceId = mLastInstanceId + 1;
  while (nextInstanceId == kSystemInstanceId
      || nextInstanceId == kBroadcastInstanceId
      || lookupAppByInstanceId(nextInstanceId) != nullptr) {
    nextInstanceId++;
  }

  mLastInstanceId = nextInstanceId;
  return nextInstanceId;
}

TimerPool& EventLoop::getTimerPool() {
  return mTimerPool;
}

Nanoapp *EventLoop::lookupAppByInstanceId(uint32_t instanceId) {
  for (Nanoapp *app : mNanoapps) {
    if (app->getInstanceId() == instanceId) {
      return app;
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
