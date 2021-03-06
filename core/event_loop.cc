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
#include "chre/platform/fatal_error.h"
#include "chre/platform/log.h"
#include "chre/platform/system_time.h"
#include "chre/util/conditional_lock_guard.h"
#include "chre/util/lock_guard.h"
#include "chre/util/system/debug_dump.h"
#include "chre/util/time.h"
#include "chre_api/chre/version.h"

namespace chre {

// Out of line declaration required for nonintegral static types
constexpr Nanoseconds EventLoop::kIntervalWakeupBucket;

namespace {

/**
 * Populates a chreNanoappInfo structure using info from the given Nanoapp
 * instance.
 *
 * @param app A potentially null pointer to the Nanoapp to read from
 * @param info The structure to populate - should not be null, but this function
 *        will handle that input
 *
 * @return true if neither app nor info were null, and info was populated
 */
bool populateNanoappInfo(const Nanoapp *app, struct chreNanoappInfo *info) {
  bool success = false;

  if (app != nullptr && info != nullptr) {
    info->appId = app->getAppId();
    info->version = app->getAppVersion();
    info->instanceId = app->getInstanceId();
    success = true;
  }

  return success;
}

}  // anonymous namespace

bool EventLoop::findNanoappInstanceIdByAppId(uint64_t appId,
                                             uint32_t *instanceId) const {
  CHRE_ASSERT(instanceId != nullptr);
  ConditionalLockGuard<Mutex> lock(mNanoappsLock, !inEventLoopThread());

  bool found = false;
  for (const UniquePtr<Nanoapp> &app : mNanoapps) {
    if (app->getAppId() == appId) {
      *instanceId = app->getInstanceId();
      found = true;
      break;
    }
  }

  return found;
}

void EventLoop::forEachNanoapp(NanoappCallbackFunction *callback, void *data) {
  ConditionalLockGuard<Mutex> lock(mNanoappsLock, !inEventLoopThread());

  for (const UniquePtr<Nanoapp> &nanoapp : mNanoapps) {
    callback(nanoapp.get(), data);
  }
}

void EventLoop::invokeMessageFreeFunction(uint64_t appId,
                                          chreMessageFreeFunction *freeFunction,
                                          void *message, size_t messageSize) {
  Nanoapp *nanoapp = lookupAppByAppId(appId);
  if (nanoapp == nullptr) {
    LOGE("Couldn't find app 0x%016" PRIx64 " for message free callback", appId);
  } else {
    auto prevCurrentApp = mCurrentApp;
    mCurrentApp = nanoapp;
    freeFunction(message, messageSize);
    mCurrentApp = prevCurrentApp;
  }
}

void EventLoop::run() {
  LOGI("EventLoop start");

  bool havePendingEvents = false;
  while (mRunning) {
    // Events are delivered in two stages: first they arrive in the inbound
    // event queue mEvents (potentially posted from another thread), then within
    // this context these events are distributed to smaller event queues
    // associated with each Nanoapp that should receive the event. Once the
    // event is delivered to all interested Nanoapps, its free callback is
    // invoked.
    if (!havePendingEvents || !mEvents.empty()) {
      if (mEvents.size() > mMaxEventPoolUsage) {
        mMaxEventPoolUsage = mEvents.size();
      }

      // mEvents.pop() will be a blocking call if mEvents.empty()
      Event *event = mEvents.pop();
      // Need size() + 1 since the to-be-processed event has already been
      // removed.
      mPowerControlManager.preEventLoopProcess(mEvents.size() + 1);
      distributeEvent(event);
    }

    havePendingEvents = deliverEvents();

    mPowerControlManager.postEventLoopProcess(mEvents.size());
  }

  // Deliver any events sitting in Nanoapps' own queues (we could drop them to
  // exit faster, but this is less code and should complete quickly under normal
  // conditions), then purge the main queue of events pending distribution. All
  // nanoapps should be prevented from sending events or messages at this point
  // via currentNanoappIsStopping() returning true.
  flushNanoappEventQueues();
  while (!mEvents.empty()) {
    freeEvent(mEvents.pop());
  }

  // Unload all running nanoapps
  while (!mNanoapps.empty()) {
    unloadNanoappAtIndex(mNanoapps.size() - 1);
  }

  LOGI("Exiting EventLoop");
}

bool EventLoop::startNanoapp(UniquePtr<Nanoapp> &nanoapp) {
  CHRE_ASSERT(!nanoapp.isNull());
  bool success = false;
  auto *eventLoopManager = EventLoopManagerSingleton::get();
  EventLoop &eventLoop = eventLoopManager->getEventLoop();
  uint32_t existingInstanceId;

  if (nanoapp.isNull()) {
    // no-op, invalid argument
  } else if (nanoapp->getTargetApiVersion() <
             CHRE_FIRST_SUPPORTED_API_VERSION) {
    LOGE("Incompatible nanoapp (target ver 0x%" PRIx32
         ", first supported ver 0x%" PRIx32 ")",
         nanoapp->getTargetApiVersion(),
         static_cast<uint32_t>(CHRE_FIRST_SUPPORTED_API_VERSION));
  } else if (eventLoop.findNanoappInstanceIdByAppId(nanoapp->getAppId(),
                                                    &existingInstanceId)) {
    LOGE("App with ID 0x%016" PRIx64
         " already exists as instance ID 0x%" PRIx32,
         nanoapp->getAppId(), existingInstanceId);
  } else if (!mNanoapps.prepareForPush()) {
    LOG_OOM();
  } else {
    nanoapp->setInstanceId(eventLoopManager->getNextInstanceId());
    LOGD("Instance ID %" PRIu32 " assigned to app ID 0x%016" PRIx64,
         nanoapp->getInstanceId(), nanoapp->getAppId());

    Nanoapp *newNanoapp = nanoapp.get();
    {
      LockGuard<Mutex> lock(mNanoappsLock);
      mNanoapps.push_back(std::move(nanoapp));
      // After this point, nanoapp is null as we've transferred ownership into
      // mNanoapps.back() - use newNanoapp to reference it
    }

    mCurrentApp = newNanoapp;
    success = newNanoapp->start();
    mCurrentApp = nullptr;
    if (!success) {
      // TODO: to be fully safe, need to purge/flush any events and messages
      // sent by the nanoapp here (but don't call nanoappEnd). For now, we just
      // destroy the Nanoapp instance.
      LOGE("Nanoapp %" PRIu32 " failed to start", newNanoapp->getInstanceId());

      // Note that this lock protects against concurrent read and modification
      // of mNanoapps, but we are assured that no new nanoapps were added since
      // we pushed the new nanoapp
      LockGuard<Mutex> lock(mNanoappsLock);
      mNanoapps.pop_back();
    } else {
      notifyAppStatusChange(CHRE_EVENT_NANOAPP_STARTED, *newNanoapp);
    }
  }

  return success;
}

bool EventLoop::unloadNanoapp(uint32_t instanceId,
                              bool allowSystemNanoappUnload) {
  bool unloaded = false;

  for (size_t i = 0; i < mNanoapps.size(); i++) {
    if (instanceId == mNanoapps[i]->getInstanceId()) {
      if (!allowSystemNanoappUnload && mNanoapps[i]->isSystemNanoapp()) {
        LOGE("Refusing to unload system nanoapp");
      } else {
        // Make sure all messages sent by this nanoapp at least have their
        // associated free callback processing pending in the event queue (i.e.
        // there are no messages pending delivery to the host)
        EventLoopManagerSingleton::get()
            ->getHostCommsManager()
            .flushMessagesSentByNanoapp(mNanoapps[i]->getAppId());

        // Distribute all inbound events we have at this time - here we're
        // interested in handling any message free callbacks generated by
        // flushMessagesSentByNanoapp()
        flushInboundEventQueue();

        // Mark that this nanoapp is stopping early, so it can't send events or
        // messages during the nanoapp event queue flush
        mStoppingNanoapp = mNanoapps[i].get();

        // Process any pending events, with the intent of ensuring that we free
        // all events generated by this nanoapp
        flushNanoappEventQueues();

        // Post the unload event now (so we can reference the Nanoapp instance
        // directly), but nanoapps won't get it until after the unload completes
        notifyAppStatusChange(CHRE_EVENT_NANOAPP_STOPPED, *mStoppingNanoapp);

        // Finally, we are at a point where there should not be any pending
        // events or messages sent by the app that could potentially reference
        // the nanoapp's memory, so we are safe to unload it
        unloadNanoappAtIndex(i);
        mStoppingNanoapp = nullptr;

        // TODO: right now we assume that the nanoapp will clean up all of its
        // resource allocations in its nanoappEnd callback (memory, sensor
        // subscriptions, etc.), otherwise we're leaking resources. We should
        // perform resource cleanup automatically here to avoid these types of
        // potential leaks.

        LOGD("Unloaded nanoapp with instanceId %" PRIu32, instanceId);
        unloaded = true;
      }
      break;
    }
  }

  return unloaded;
}

void EventLoop::postEventOrDie(uint16_t eventType, void *eventData,
                               chreEventCompleteFunction *freeCallback,
                               uint32_t targetInstanceId,
                               uint16_t targetGroupMask) {
  if (mRunning) {
    if (!allocateAndPostEvent(eventType, eventData, freeCallback,
                              kSystemInstanceId, targetInstanceId,
                              targetGroupMask)) {
      FATAL_ERROR("Failed to post critical system event 0x%" PRIx16, eventType);
    }
  } else if (freeCallback != nullptr) {
    freeCallback(eventType, eventData);
  }
}

bool EventLoop::postSystemEvent(uint16_t eventType, void *eventData,
                                SystemEventCallbackFunction *callback,
                                void *extraData) {
  if (mRunning) {
    Event *event =
        mEventPool.allocate(eventType, eventData, callback, extraData);

    if (event == nullptr || !mEvents.push(event)) {
      FATAL_ERROR("Failed to post critical system event 0x%" PRIx16, eventType);
    }
    return true;
  }
  return false;
}

bool EventLoop::postLowPriorityEventOrFree(
    uint16_t eventType, void *eventData,
    chreEventCompleteFunction *freeCallback, uint32_t senderInstanceId,
    uint32_t targetInstanceId, uint16_t targetGroupMask) {
  bool eventPosted = false;

  if (mRunning) {
    if (mEventPool.getFreeBlockCount() > kMinReservedHighPriorityEventCount) {
      eventPosted = allocateAndPostEvent(eventType, eventData, freeCallback,
                                         senderInstanceId, targetInstanceId,
                                         targetGroupMask);
      if (!eventPosted) {
        LOGE("Failed to allocate event 0x%" PRIx16 " to instanceId %" PRIu32,
             eventType, targetInstanceId);
      }
    }
  }

  if (!eventPosted && freeCallback != nullptr) {
    freeCallback(eventType, eventData);
  }

  return eventPosted;
}

void EventLoop::stop() {
  auto callback = [](uint16_t /*type*/, void *data, void * /*extraData*/) {
    auto *obj = static_cast<EventLoop *>(data);
    obj->onStopComplete();
  };

  // Stop accepting new events and tell the main loop to finish
  postSystemEvent(static_cast<uint16_t>(SystemCallbackType::Shutdown),
                  /*eventData=*/this, callback, /*extraData=*/nullptr);
}

void EventLoop::onStopComplete() {
  mRunning = false;
}

Nanoapp *EventLoop::findNanoappByInstanceId(uint32_t instanceId) const {
  ConditionalLockGuard<Mutex> lock(mNanoappsLock, !inEventLoopThread());
  return lookupAppByInstanceId(instanceId);
}

bool EventLoop::populateNanoappInfoForAppId(
    uint64_t appId, struct chreNanoappInfo *info) const {
  ConditionalLockGuard<Mutex> lock(mNanoappsLock, !inEventLoopThread());
  Nanoapp *app = lookupAppByAppId(appId);
  return populateNanoappInfo(app, info);
}

bool EventLoop::populateNanoappInfoForInstanceId(
    uint32_t instanceId, struct chreNanoappInfo *info) const {
  ConditionalLockGuard<Mutex> lock(mNanoappsLock, !inEventLoopThread());
  Nanoapp *app = lookupAppByInstanceId(instanceId);
  return populateNanoappInfo(app, info);
}

bool EventLoop::currentNanoappIsStopping() const {
  return (mCurrentApp == mStoppingNanoapp || !mRunning);
}

void EventLoop::logStateToBuffer(DebugDumpWrapper &debugDump) const {
  debugDump.print("\nEvent Loop:\n");
  debugDump.print("  Max event pool usage: %zu/%zu\n", mMaxEventPoolUsage,
                  kMaxEventCount);

  Nanoseconds timeSince =
      SystemTime::getMonotonicTime() - mTimeLastWakeupBucketCycled;
  uint64_t timeSinceMins =
      timeSince.toRawNanoseconds() / kOneMinuteInNanoseconds;
  uint64_t durationMins =
      kIntervalWakeupBucket.toRawNanoseconds() / kOneMinuteInNanoseconds;
  debugDump.print("  Nanoapp host wakeup tracking: cycled %" PRIu64
                  "mins ago, bucketDuration=%" PRIu64 "mins\n",
                  timeSinceMins, durationMins);

  debugDump.print("\nNanoapps:\n");
  for (const UniquePtr<Nanoapp> &app : mNanoapps) {
    app->logStateToBuffer(debugDump);
  }
}

bool EventLoop::allocateAndPostEvent(uint16_t eventType, void *eventData,
                                     chreEventCompleteFunction *freeCallback,
                                     uint32_t senderInstanceId,
                                     uint32_t targetInstanceId,
                                     uint16_t targetGroupMask) {
  bool success = false;

  Event *event =
      mEventPool.allocate(eventType, eventData, freeCallback, senderInstanceId,
                          targetInstanceId, targetGroupMask);
  if (event != nullptr) {
    success = mEvents.push(event);
  }

  return success;
}

bool EventLoop::deliverEvents() {
  bool havePendingEvents = false;

  // Do one loop of round-robin. We might want to have some kind of priority or
  // time sharing in the future, but this should be good enough for now.
  for (const UniquePtr<Nanoapp> &app : mNanoapps) {
    if (app->hasPendingEvent()) {
      havePendingEvents |= deliverNextEvent(app);
    }
  }

  return havePendingEvents;
}

bool EventLoop::deliverNextEvent(const UniquePtr<Nanoapp> &app) {
  // TODO: cleaner way to set/clear this? RAII-style?
  mCurrentApp = app.get();
  Event *event = app->processNextEvent();
  mCurrentApp = nullptr;

  if (event->isUnreferenced()) {
    freeEvent(event);
  }

  return app->hasPendingEvent();
}

void EventLoop::distributeEvent(Event *event) {
  for (const UniquePtr<Nanoapp> &app : mNanoapps) {
    if ((event->targetInstanceId == chre::kBroadcastInstanceId &&
         app->isRegisteredForBroadcastEvent(event->eventType,
                                            event->targetAppGroupMask)) ||
        event->targetInstanceId == app->getInstanceId()) {
      app->postEvent(event);
    }
  }

  if (event->isUnreferenced()) {
    // Log if an event unicast to a nanoapp isn't delivered, as this is could be
    // a bug (e.g. something isn't properly keeping track of when nanoapps are
    // unloaded), though it could just be a harmless transient issue (e.g. race
    // condition with nanoapp unload, where we post an event to a nanoapp just
    // after queues are flushed while it's unloading)
    if (event->targetInstanceId != kBroadcastInstanceId &&
        event->targetInstanceId != kSystemInstanceId) {
      LOGW("Dropping event 0x%" PRIx16 " from instanceId %" PRIu32 "->%" PRIu32,
           event->eventType, event->senderInstanceId, event->targetInstanceId);
    }
    freeEvent(event);
  }
}

void EventLoop::flushInboundEventQueue() {
  while (!mEvents.empty()) {
    distributeEvent(mEvents.pop());
  }
}

void EventLoop::flushNanoappEventQueues() {
  while (deliverEvents())
    ;
}

void EventLoop::freeEvent(Event *event) {
  if (event->hasFreeCallback()) {
    // TODO: find a better way to set the context to the creator of the event
    mCurrentApp = lookupAppByInstanceId(event->senderInstanceId);
    event->invokeFreeCallback();
    mCurrentApp = nullptr;
  }

  mEventPool.deallocate(event);
}

Nanoapp *EventLoop::lookupAppByAppId(uint64_t appId) const {
  for (const UniquePtr<Nanoapp> &app : mNanoapps) {
    if (app->getAppId() == appId) {
      return app.get();
    }
  }

  return nullptr;
}

Nanoapp *EventLoop::lookupAppByInstanceId(uint32_t instanceId) const {
  // The system instance ID always has nullptr as its Nanoapp pointer, so can
  // skip iterating through the nanoapp list for that case
  if (instanceId != kSystemInstanceId) {
    for (const UniquePtr<Nanoapp> &app : mNanoapps) {
      if (app->getInstanceId() == instanceId) {
        return app.get();
      }
    }
  }

  return nullptr;
}

void EventLoop::notifyAppStatusChange(uint16_t eventType,
                                      const Nanoapp &nanoapp) {
  auto *info = memoryAlloc<chreNanoappInfo>();
  if (info == nullptr) {
    LOG_OOM();
  } else {
    info->appId = nanoapp.getAppId();
    info->version = nanoapp.getAppVersion();
    info->instanceId = nanoapp.getInstanceId();

    postEventOrDie(eventType, info, freeEventDataCallback);
  }
}

void EventLoop::unloadNanoappAtIndex(size_t index) {
  const UniquePtr<Nanoapp> &nanoapp = mNanoapps[index];

  // Lock here to prevent the nanoapp instance from being accessed between the
  // time it is ended and fully erased
  LockGuard<Mutex> lock(mNanoappsLock);

  // Let the app know it's going away
  mCurrentApp = nanoapp.get();
  nanoapp->end();
  mCurrentApp = nullptr;

  // Destroy the Nanoapp instance
  mNanoapps.erase(index);
}

void EventLoop::handleNanoappWakeupBuckets() {
  Nanoseconds now = SystemTime::getMonotonicTime();
  Nanoseconds duration = now - mTimeLastWakeupBucketCycled;
  if (duration > kIntervalWakeupBucket) {
    size_t numBuckets = static_cast<size_t>(
        duration.toRawNanoseconds() / kIntervalWakeupBucket.toRawNanoseconds());
    mTimeLastWakeupBucketCycled = now;
    for (auto &nanoapp : mNanoapps) {
      nanoapp->cycleWakeupBuckets(numBuckets);
    }
  }
}

}  // namespace chre
