/*
 * Copyright (C) 2021 The Android Open Source Project
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

#ifndef CHRE_SIMULATION_TEST_EVENT_QUEUE_H_
#define CHRE_SIMULATION_TEST_EVENT_QUEUE_H_

#include <gtest/gtest.h>

#include <cinttypes>

#include "chre/util/fixed_size_blocking_queue.h"
#include "chre/util/non_copyable.h"
#include "chre/util/singleton.h"
#include "chre_api/chre/event.h"

namespace chre {

//! A test event type to indicate the test nanoapp has loaded.
#define CHRE_EVENT_SIMULATION_TEST_NANOAPP_LOADED (CHRE_EVENT_FIRST_USER_VALUE)

//! A test event type to indicate the test has timed out, and should abort.
#define CHRE_EVENT_SIMULATION_TEST_TIMEOUT (CHRE_EVENT_FIRST_USER_VALUE + 1)

/**
 * A class that monitors events for the test to consume.
 *
 * This class can be used as an execution barrier for the test, i.e. waiting
 * for a specific event to occur. The barrier is done through the semantics of
 * CHRE events, and can be used e.g. for nanoapps to redirect incoming events
 * using pushEvent().
 *
 * The main test thread can then wait for this event using waitForEvent().
 *
 * Note 1) pushEvent() can also be invoked outside the nanoapp, for instance
 * using deferred system callbacks.
 * Note 2) The CHRE_EVENT_SIMULATION_TEST_TIMEOUT event type can be used to
 * abort the test due to a timeout (this usage is recommended in order to avoid
 * the test framework from stalling).
 */
class TestEventQueue : public NonCopyable {
 public:
  void pushEvent(uint16_t eventType) {
    mQueue.push(eventType);
  }

  void waitForEvent(uint16_t eventType) {
    while (true) {
      uint16_t type = mQueue.pop();
      LOGD("Got event type 0x%" PRIx16, eventType);
      ASSERT_NE(type, CHRE_EVENT_SIMULATION_TEST_TIMEOUT);
      if (type == eventType) {
        break;
      }
    }
  }

 private:
  static const size_t kQueueCapacity = 64;
  FixedSizeBlockingQueue<uint16_t, kQueueCapacity> mQueue;
};

//! Provide an alias to the TestEventQueue singleton.
typedef Singleton<TestEventQueue> TestEventQueueSingleton;

//! Extern the explicit TestEventQueueSingleton to force non-inline method
//! calls.
extern template class Singleton<TestEventQueue>;

}  // namespace chre

#endif  // CHRE_SIMULATION_TEST_EVENT_QUEUE_H_
