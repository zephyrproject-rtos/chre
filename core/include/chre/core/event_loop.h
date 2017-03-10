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

#ifndef CHRE_CORE_EVENT_LOOP_H_
#define CHRE_CORE_EVENT_LOOP_H_

#include "chre/core/event.h"
#include "chre/core/nanoapp.h"
#include "chre/core/timer_pool.h"
#include "chre/platform/mutex.h"
#include "chre/util/dynamic_vector.h"
#include "chre/util/fixed_size_blocking_queue.h"
#include "chre/util/non_copyable.h"
#include "chre/util/synchronized_memory_pool.h"

namespace chre {

/**
 * The EventLoop represents a single thread of execution that is shared among
 * zero or more nanoapps. As the name implies, the EventLoop is built around a
 * loop that delivers events to the nanoapps managed within for processing.
 */
class EventLoop : public NonCopyable {
 public:
  /**
   * Setup the event loop.
   */
  EventLoop();

  /**
   * Searches the set of nanoapps managed by this EventLoop for one with the
   * given app ID. If found, provides its instance ID, which can be used to send
   * events to the app.
   *
   * This function is safe to call from any thread.
   *
   * @param appId The nanoapp identifier to search for.
   * @param instanceId If this function returns true, will be populated with the
   *        instanceId associated with the given appId; otherwise unmodified.
   *        Must not be null.
   * @return true if the given app ID was found and instanceId was populated
   */
  bool findNanoappInstanceIdByAppId(uint64_t appId, uint32_t *instanceId);

  /**
   * Starts a nanoapp by invoking the start entry point. If this is successful,
   * the app is managed by the event loop and the pointer passed in must remain
   * valid.
   *
   * @param A pointer to the nanoapp to start.
   * @return True if the app was started successfully.
   */
  bool startNanoapp(Nanoapp *nanoapp);

  /**
   * Stops a nanoapp by invoking the stop entry point. The nanoapp passed in
   * must have been previously started by the startNanoapp method.
   *
   * @param A pointer to the nanoapp to stop.
   */
  void stopNanoapp(Nanoapp *nanoapp);

  /**
   * Executes the loop that blocks on the event queue and delivers received
   * events to nanoapps. Only returns after stop() is called (from another
   * context).
   */
  void run();

  /**
   * Signals the event loop currently executing in run() to exit gracefully at
   * the next available opportunity. This function is thread-safe.
   */
  void stop();

  /**
   * Posts an event to a nanoapp that is currently running (or all nanoapps if
   * the target instance ID is kBroadcastInstanceId).
   *
   * This function is safe to call from any thread.
   *
   * @param The type of data being posted.
   * @param The data being posted.
   * @param The callback to invoke when the event is no longer needed.
   * @param The instance ID of the sender of this event.
   * @param The instance ID of the destination of this event.
   *
   * @return true if the event was successfully added to the queue
   */
  bool postEvent(uint16_t eventType, void *eventData,
                 chreEventCompleteFunction *freeCallback,
                 uint32_t senderInstanceId = kSystemInstanceId,
                 uint32_t targetInstanceId = kBroadcastInstanceId);

  /**
   * Returns a pointer to the currently executing Nanoapp, or nullptr if none is
   * currently executing. Must only be called from within the thread context
   * associated with this EventLoop.
   *
   * @return the currently executing nanoapp, or nullptr
   */
  Nanoapp *getCurrentNanoapp() const;

  /**
   * Returns a guaranteed unique instance ID that can be used to construct a
   * nanoapp.
   *
   * @return a unique instance ID to assign to a nanoapp.
   */
  // TODO: move this to EventLoopManager as it must be unique across all event
  // loops; we currently really only support one EventLoop so it's OK for now
  uint32_t getNextInstanceId();

  /**
   * Obtains the TimerPool associated with this event loop.
   *
   * @return The timer pool owned by this event loop.
   */
  TimerPool& getTimerPool();

  /**
   * Searches the set of nanoapps managed by this EventLoop for one with the
   * given instance ID.
   *
   * This function is safe to call from any thread.
   *
   * @param instanceId The nanoapp instance ID to search for.
   * @return a pointer to the found nanoapp or nullptr if no match was found.
   */
  Nanoapp *findNanoappByInstanceId(uint32_t instanceId);

 private:
  //! The maximum number of events that can be active in the system.
  static constexpr size_t kMaxEventCount = 1024;

  //! The maximum number of events that are awaiting to be scheduled. These
  //! events are in a queue to be distributed to apps.
  static constexpr size_t kMaxUnscheduledEventCount = 1024;

  //! The instance ID that was previously generated by getNextInstanceId.
  uint32_t mLastInstanceId = kSystemInstanceId;

  //! The memory pool to allocate incoming events from.
  SynchronizedMemoryPool<Event, kMaxEventCount> mEventPool;

  //! The timer used schedule timed events for tasks running in this event loop.
  TimerPool mTimerPool;

  //! The list of nanoapps managed by this event loop.
  DynamicVector<Nanoapp *> mNanoapps;

  //! This lock *must* be held whenever we:
  //!   (1) make changes to the mNanoapps vector, or
  //!   (2) read the mNanoapps vector from a thread other than the one
  //!       associated with this EventLoop
  //! It is not necessary to acquire the lock when reading mNanoapps from within
  //! the thread context of this EventLoop.
  Mutex mNanoappsLock;

  //! The blocking queue of incoming events from the system that have not been
  //!  distributed out to apps yet.
  FixedSizeBlockingQueue<Event *, kMaxUnscheduledEventCount> mEvents;

  // TODO: should probably be atomic to be fully correct
  volatile bool mRunning = false;

  Nanoapp *mCurrentApp = nullptr;

  /**
   * Call after when an Event has been delivered to all intended recipients.
   * Invokes the event's free callback (if given) and releases resources.
   *
   * @param event The event to be freed
   */
  void freeEvent(Event *event);

  /**
   * Finds a Nanoapp with the given instanceId.
   *
   * Only safe to call within this EventLoop's thread.
   *
   * @param instanceId Nanoapp instance identifier
   * @return Nanoapp with the given instanceId, or nullptr if not found
   */
  Nanoapp *lookupAppByInstanceId(uint32_t instanceId);
};

}  // namespace chre

#endif  // CHRE_CORE_EVENT_LOOP_H_
