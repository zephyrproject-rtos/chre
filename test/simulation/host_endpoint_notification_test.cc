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

#include <thread>

#include "chre/core/event_loop_manager.h"
#include "chre/core/host_notifications.h"
#include "chre/platform/log.h"
#include "chre_api/chre/event.h"
#include "test_event_queue.h"
#include "test_util.h"

namespace chre {

namespace {

//! The host endpoint ID to use for this test.
constexpr uint16_t kHostEndpointId = 123;

bool start() {
  chreConfigureHostEndpointNotifications(kHostEndpointId, true /* enable */);
  TestEventQueueSingleton::get()->pushEvent(
      CHRE_EVENT_SIMULATION_TEST_NANOAPP_LOADED);
  return true;
}

void handleEvent(uint32_t /* senderInstanceId */, uint16_t eventType,
                 const void * /* eventData */) {
  TestEventQueueSingleton::get()->pushEvent(eventType);
}

void end() {
  chreConfigureHostEndpointNotifications(kHostEndpointId, false /* enable */);
}

}  // anonymous namespace

/**
 * Verifies basic functionality of chreConfigureHostEndpointNotifications.
 */
// TODO(b/194287786): Add more test cases:
// 1) Host endpoint isn't registered.
// 2) Host endpoint is unregistered twice.
TEST_F(TestBase, HostEndpointDisconnectedTest) {
  constexpr uint64_t kAppId = 0x0123456789abcdef;
  constexpr uint32_t kAppVersion = 0;
  constexpr uint32_t kAppPerms = 0;

  postHostEndpointConnected(kHostEndpointId);

  UniquePtr<Nanoapp> nanoapp = createStaticNanoapp(
      "Test nanoapp", kAppId, kAppVersion, kAppPerms, start, handleEvent, end);
  EventLoopManagerSingleton::get()->deferCallback(
      SystemCallbackType::FinishLoadingNanoapp, std::move(nanoapp),
      testFinishLoadingNanoappCallback);
  waitForEvent(CHRE_EVENT_SIMULATION_TEST_NANOAPP_LOADED);

  postHostEndpointDisconnected(kHostEndpointId);
  waitForEvent(CHRE_EVENT_HOST_ENDPOINT_NOTIFICATION);

  // TODO(b/194287786): Check contents of host endpoint notification event.
}

}  // namespace chre
