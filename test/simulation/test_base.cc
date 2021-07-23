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
 */
void TestBase::SetUp() {
  chre::PlatformLogSingleton::init();
  chre::init();
  EventLoopManagerSingleton::get()->lateInit();

  mChreThread = std::thread(
      []() { EventLoopManagerSingleton::get()->getEventLoop().run(); });
}

void TestBase::TearDown() {
  EventLoopManagerSingleton::get()->getEventLoop().stop();
  mChreThread.join();

  chre::deinit();
  chre::PlatformLogSingleton::deinit();
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

}  // namespace chre
