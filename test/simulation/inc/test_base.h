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

#ifndef CHRE_SIMULATION_TEST_BASE_H_
#define CHRE_SIMULATION_TEST_BASE_H_

#include <gtest/gtest.h>

#include <thread>

#include "chre/util/time.h"
#include "test_event_queue.h"

namespace chre {

/*
 * A base class for all CHRE simulated tests.
 */
class TestBase : public testing::Test {
 protected:
  void SetUp() override;
  void TearDown() override;

  /**
   * This method can be overridden in a derived class if desired.
   *
   * @return The total runtime allowed for the entire test.
   */
  virtual uint64_t getTimeoutNs() const {
    return 5 * kOneSecondInNanoseconds;
  }

  /**
   * A convenience method to invoke waitForEvent() for the TestEventQueue
   * singleton.
   *
   * @param eventType The event type to wait for.
   */
  void waitForEvent(uint16_t eventType) {
    TestEventQueueSingleton::get()->waitForEvent(eventType);
  }

  std::thread mChreThread;
};

}  // namespace chre

#endif  // CHRE_SIMULATION_TEST_BASE_H_
