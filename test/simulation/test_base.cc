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

#include "test_base.h"

#include <gtest/gtest.h>

#include "chre/core/event_loop_manager.h"
#include "chre/core/init.h"
#include "chre/platform/linux/platform_log.h"
#include "chre_api/chre/version.h"
#include "test_util.h"

namespace chre {

namespace {

bool gNanoappStarted = false;

}  // anonymous namespace

/**
 * This base class initializes and runs the event loop.
 *
 * This test framework makes use of the TestEventQueue as a primary method
 * of a test execution barrier (see its documentation for details). To simplify
 * the test execution flow, it is encouraged that any communication between
 * threads (e.g. a nanoapp and the main test thread) through this
 * TestEventQueue. In this way, we can design simulation tests in a way that
 * validates an expected sequence of events in a well-defined manner.
 *
 * To avoid the test from potentially stalling, we also push a timeout event
 * to the TestEventQueue once a fixed timeout has elapsed since the start of
 * this test.
 */
void TestBase::SetUp() {
  TestEventQueueSingleton::init();
  chre::PlatformLogSingleton::init();
  chre::init();
  EventLoopManagerSingleton::get()->lateInit();

  mChreThread = std::thread(
      []() { EventLoopManagerSingleton::get()->getEventLoop().run(); });

  auto callback = [](uint16_t /*type*/, void * /* data */,
                     void * /*extraData*/) {
    LOGE("Test timed out ...");
    TestEventQueueSingleton::get()->pushEvent(
        CHRE_EVENT_SIMULATION_TEST_TIMEOUT);
  };
  TimerHandle handle = EventLoopManagerSingleton::get()->setDelayedCallback(
      SystemCallbackType::DelayedFatalError, nullptr /* data */, callback,
      Nanoseconds(getTimeoutNs()));
  ASSERT_NE(handle, CHRE_TIMER_INVALID);
}

void TestBase::TearDown() {
  EventLoopManagerSingleton::get()->getEventLoop().stop();
  mChreThread.join();

  chre::deinit();
  chre::PlatformLogSingleton::deinit();
  TestEventQueueSingleton::deinit();
}

/**
 * A basic test to ensure nanoapps can load and start.
 */
TEST_F(TestBase, SimpleNanoappTest) {
  constexpr uint64_t kAppId = 0x0123456789abcdef;
  constexpr uint32_t kAppVersion = 0;
  constexpr uint32_t kAppPerms = 0;

  gNanoappStarted = false;
  chreNanoappStartFunction *start = []() {
    gNanoappStarted = true;
    return true;
  };

  UniquePtr<Nanoapp> nanoapp =
      createStaticNanoapp("Test nanoapp", kAppId, kAppVersion, kAppPerms, start,
                          defaultNanoappHandleEvent, defaultNanoappEnd);

  EventLoopManagerSingleton::get()->getEventLoop().startNanoapp(nanoapp);

  ASSERT_TRUE(gNanoappStarted);
}

// Explicitly instantiate the TestEventQueueSingleton to reduce codesize.
template class Singleton<TestEventQueue>;

}  // namespace chre
