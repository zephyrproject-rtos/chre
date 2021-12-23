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
#include "chre_api/chre/ble.h"
#include "test_util.h"

namespace chre {

namespace {

uint32_t gBleCapabilities = 0;
uint32_t gBleFilterCapabilities = 0;

bool start() {
  gBleCapabilities = chreBleGetCapabilities();
  gBleFilterCapabilities = chreBleGetFilterCapabilities();
  TestEventQueueSingleton::get()->pushEvent(
      CHRE_EVENT_SIMULATION_TEST_NANOAPP_LOADED);
  return true;
}

}  // namespace

/**
 * This test verifies that a nanoapp can query for BLE capabilities and filter
 * capabilities. Note that a nanoapp does not require BLE permissions to use
 * these APIs.
 */
TEST_F(TestBase, BleCapabilitiesTest) {
  constexpr uint64_t kAppId = 0x0123456789abcdef;
  constexpr uint32_t kAppVersion = 0;
  constexpr uint32_t kAppPerms = NanoappPermissions::CHRE_PERMS_NONE;

  UniquePtr<Nanoapp> nanoapp =
      createStaticNanoapp("Test nanoapp", kAppId, kAppVersion, kAppPerms, start,
                          defaultNanoappHandleEvent, defaultNanoappEnd);
  EventLoopManagerSingleton::get()->deferCallback(
      SystemCallbackType::FinishLoadingNanoapp, std::move(nanoapp),
      testFinishLoadingNanoappCallback);
  waitForEvent(CHRE_EVENT_SIMULATION_TEST_NANOAPP_LOADED);

  ASSERT_EQ(gBleCapabilities,
            CHRE_BLE_CAPABILITIES_SCAN |
                CHRE_BLE_CAPABILITIES_SCAN_RESULT_BATCHING |
                CHRE_BLE_CAPABILITIES_SCAN_FILTER_BEST_EFFORT);

  ASSERT_EQ(gBleFilterCapabilities,
            CHRE_BLE_FILTER_CAPABILITIES_RSSI |
                CHRE_BLE_FILTER_CAPABILITIES_SERVICE_DATA_UUID);
}

}  // namespace chre
