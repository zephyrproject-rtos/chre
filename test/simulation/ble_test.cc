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
#include "chre/core/settings.h"
#include "chre/platform/fatal_error.h"
#include "chre/platform/linux/pal_ble.h"
#include "chre/util/dynamic_vector.h"
#include "chre_api/chre/ble.h"
#include "chre_api/chre/user_settings.h"
#include "test_util.h"

namespace chre {

#define CHRE_EVENT_BLE_START_SCAN CHRE_SPECIFIC_SIMULATION_TEST_EVENT_ID(0)
#define CHRE_EVENT_BLE_STOP_SCAN CHRE_SPECIFIC_SIMULATION_TEST_EVENT_ID(1)
#define CHRE_EVENT_BLE_FUNCTION_DISABLED_ERROR \
  CHRE_SPECIFIC_SIMULATION_TEST_EVENT_ID(2)
#define CHRE_EVENT_BLE_UNKNOWN_ERROR_CODE \
  CHRE_SPECIFIC_SIMULATION_TEST_EVENT_ID(3)
#define CHRE_EVENT_BLE_SETTING_ENABLED CHRE_SPECIFIC_SIMULATION_TEST_EVENT_ID(4)
#define CHRE_EVENT_BLE_SETTING_DISABLED \
  CHRE_SPECIFIC_SIMULATION_TEST_EVENT_ID(5)

namespace {

constexpr uint64_t gAppId = 0x0123456789abcdef;
constexpr uint32_t gAppVersion = 0;
constexpr uint32_t gAppPerms = NanoappPermissions::CHRE_PERMS_BLE;

uint32_t gBleCapabilities = 0;
uint32_t gBleFilterCapabilities = 0;

bool capabilitiesStart() {
  gBleCapabilities = chreBleGetCapabilities();
  gBleFilterCapabilities = chreBleGetFilterCapabilities();
  TestEventQueueSingleton::get()->pushEvent(
      CHRE_EVENT_SIMULATION_TEST_NANOAPP_LOADED);
  return true;
}

uint16_t getTestEventType(uint16_t eventType, const void *eventData) {
  uint16_t type = eventType;
  if (eventType == CHRE_EVENT_BLE_ASYNC_RESULT) {
    auto *event = static_cast<const struct chreAsyncResult *>(eventData);
    type = (event->requestType == CHRE_BLE_REQUEST_TYPE_START_SCAN)
               ? CHRE_EVENT_BLE_START_SCAN
               : CHRE_EVENT_BLE_STOP_SCAN;
    if (event->errorCode == CHRE_ERROR_FUNCTION_DISABLED) {
      type = CHRE_EVENT_BLE_FUNCTION_DISABLED_ERROR;
    } else if (event->errorCode != CHRE_ERROR_NONE) {
      type = CHRE_EVENT_BLE_UNKNOWN_ERROR_CODE;
    }
  } else if (eventType == CHRE_EVENT_SETTING_CHANGED_BLE_AVAILABLE) {
    auto *event = static_cast<const chreUserSettingChangedEvent *>(eventData);
    type = (event->settingState == CHRE_USER_SETTING_STATE_ENABLED)
               ? CHRE_EVENT_BLE_SETTING_ENABLED
               : CHRE_EVENT_BLE_SETTING_DISABLED;
  }
  return type;
}

struct EventCounts {
  uint8_t start;
  uint8_t stop;
  uint8_t advertisment;
};

EventCounts gExpectedCounts{0, 0, 0};
EventCounts gCounts{0, 0, 0};

// Populate the minimum expected BLE scan events for a test.
void setExpectedEventCounts(uint8_t start = 1, uint8_t advertisement = 0,
                            uint8_t stop = 0) {
  gCounts = {0, 0, 0};
  gExpectedCounts.start = start;
  gExpectedCounts.advertisment = advertisement;
  gExpectedCounts.stop = stop;
}

void populateExpectedEvents(DynamicVector<uint16_t> &expectedEvents) {
  expectedEvents.push_back(CHRE_EVENT_SIMULATION_TEST_NANOAPP_LOADED);
  for (uint8_t i = 0; i < gExpectedCounts.start; i++) {
    expectedEvents.push_back(CHRE_EVENT_BLE_START_SCAN);
  }
  for (uint8_t i = 0; i < gExpectedCounts.advertisment; i++) {
    expectedEvents.push_back(CHRE_EVENT_BLE_ADVERTISEMENT);
  }
  for (uint8_t i = 0; i < gExpectedCounts.stop; i++) {
    expectedEvents.push_back(CHRE_EVENT_BLE_STOP_SCAN);
  }
}

void setupScanTest(uint8_t start, uint8_t advertisement, uint8_t stop,
                   DynamicVector<uint16_t> &expectedEvents) {
  setExpectedEventCounts(start, advertisement, stop);
  populateExpectedEvents(expectedEvents);
}

void handleEvent(uint32_t /* senderInstanceId */, uint16_t eventType,
                 const void *eventData) {
  uint16_t testEvent = getTestEventType(eventType, eventData);
  TestEventQueueSingleton::get()->pushEvent(testEvent);
  if (testEvent == CHRE_EVENT_BLE_START_SCAN) {
    gCounts.start++;
    // Delay incrementing advertisement events until all start events are
    // processed.
  } else if (testEvent == CHRE_EVENT_BLE_ADVERTISEMENT &&
             gCounts.start >= gExpectedCounts.start) {
    gCounts.advertisment++;
  } else if (testEvent == CHRE_EVENT_BLE_STOP_SCAN) {
    gCounts.stop++;
  } else if (testEvent == CHRE_EVENT_BLE_UNKNOWN_ERROR_CODE) {
    FATAL_ERROR("Unexpected ble error");
  }

  // Determine next API call based on event counts.
  if (gCounts.start < gExpectedCounts.start) {
    chreBleStartScanAsync(CHRE_BLE_SCAN_MODE_BACKGROUND, 0, nullptr);
  } else if (gCounts.advertisment < gExpectedCounts.advertisment) {
  } else if (gCounts.stop < gExpectedCounts.stop) {
    chreBleStopScanAsync();
  }
}

bool startBleNanoapp() {
  TestEventQueueSingleton::get()->pushEvent(
      CHRE_EVENT_SIMULATION_TEST_NANOAPP_LOADED);
  if (gExpectedCounts.start > 0) {
    chreBleStartScanAsync(CHRE_BLE_SCAN_MODE_BACKGROUND, 0, nullptr);
  } else if (gExpectedCounts.stop > 0) {
    chreBleStopScanAsync();
  }
  chreUserSettingConfigureEvents(CHRE_USER_SETTING_BLE_AVAILABLE,
                                 true /* enable */);
  return true;
}

void endBleNanoapp() {
  chreUserSettingConfigureEvents(CHRE_USER_SETTING_BLE_AVAILABLE,
                                 false /* enable */);
  TestEventQueueSingleton::get()->pushEvent(
      CHRE_EVENT_SIMULATION_TEST_NANOAPP_UNLOADED);
}

}  // namespace

/**
 * This test verifies that a nanoapp can query for BLE capabilities and filter
 * capabilities. Note that a nanoapp does not require BLE permissions to use
 * these APIs.
 */
TEST_F(TestBase, BleCapabilitiesTest) {
  loadNanoapp("Test nanoapp", gAppId, gAppVersion,
              NanoappPermissions::CHRE_PERMS_NONE, capabilitiesStart,
              defaultNanoappHandleEvent, defaultNanoappEnd);
  waitForEvent(CHRE_EVENT_SIMULATION_TEST_NANOAPP_LOADED);
  ASSERT_EQ(gBleCapabilities,
            CHRE_BLE_CAPABILITIES_SCAN |
                CHRE_BLE_CAPABILITIES_SCAN_RESULT_BATCHING |
                CHRE_BLE_CAPABILITIES_SCAN_FILTER_BEST_EFFORT);
  ASSERT_EQ(gBleFilterCapabilities,
            CHRE_BLE_FILTER_CAPABILITIES_RSSI |
                CHRE_BLE_FILTER_CAPABILITIES_SERVICE_DATA_UUID);
}

/**
 * This test validates the case in which a nanoapp starts a scan, receives at
 * least one advertisement event, and stops a scan.
 */
TEST_F(TestBase, BleSimpleScanTest) {
  DynamicVector<uint16_t> expectedEvents;
  setupScanTest(1 /* start */, 1 /* advertisement */, 1 /* stop */,
                expectedEvents);
  loadNanoapp("Test nanoapp", gAppId, gAppVersion, gAppPerms, startBleNanoapp,
              handleEvent, endBleNanoapp);
  for (size_t i = 0; i < expectedEvents.size(); i++) {
    waitForEvent(expectedEvents[i]);
  }
}

/**
 * This test validates that a nanoapp can start a scan twice and the platform
 * will be enabled.
 */
TEST_F(TestBase, BleStartTwiceScanTest) {
  DynamicVector<uint16_t> expectedEvents;
  setupScanTest(2 /* start */, 1 /* advertisement */, 1 /* stop */,
                expectedEvents);
  loadNanoapp("Test nanoapp", gAppId, gAppVersion, gAppPerms, startBleNanoapp,
              handleEvent, endBleNanoapp);
  for (size_t i = 0; i < expectedEvents.size(); i++) {
    waitForEvent(expectedEvents[i]);
  }
}

/**
 * This test validates that a nanoapp can request to stop a scan twice without
 * any ongoing scan existing. It asserts that the nanoapp did not receive any
 * advertisment events because a scan was never started.
 */
TEST_F(TestBase, BleStopTwiceScanTest) {
  DynamicVector<uint16_t> expectedEvents;
  setupScanTest(0 /* start */, 0 /* advertisement */, 2 /* stop */,
                expectedEvents);
  loadNanoapp("Test nanoapp", gAppId, gAppVersion, gAppPerms, startBleNanoapp,
              handleEvent, endBleNanoapp);
  for (size_t i = 0; i < expectedEvents.size(); i++) {
    waitForEvent(expectedEvents[i]);
  }
  unloadNanoapp(gAppId);
  waitForEvent(CHRE_EVENT_SIMULATION_TEST_NANOAPP_UNLOADED);
  ASSERT_EQ(gCounts.advertisment, 0);
}

/**
 * This test verifies the following BLE settings behavior:
 * 1) Nanoapp makes BLE scan request
 * 2) Toggle BLE setting -> disabled
 * 3) Toggle BLE setting -> enabled.
 * 4) Verify things resume.
 */
TEST_F(TestBase, BleSettingChangeTest) {
  setExpectedEventCounts();
  loadNanoapp("Test nanoapp", gAppId, gAppVersion, gAppPerms, startBleNanoapp,
              handleEvent, endBleNanoapp);
  waitForEvent(CHRE_EVENT_SIMULATION_TEST_NANOAPP_LOADED);
  waitForEvent(CHRE_EVENT_BLE_START_SCAN);
  waitForEvent(CHRE_EVENT_BLE_ADVERTISEMENT);

  EventLoopManagerSingleton::get()->getSettingManager().postSettingChange(
      Setting::BLE_AVAILABLE, false /* enabled */);
  waitForEvent(CHRE_EVENT_BLE_SETTING_DISABLED);
  ASSERT_FALSE(
      EventLoopManagerSingleton::get()->getSettingManager().getSettingEnabled(
          Setting::BLE_AVAILABLE));
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_FALSE(chrePalIsBleEnabled());

  EventLoopManagerSingleton::get()->getSettingManager().postSettingChange(
      Setting::BLE_AVAILABLE, true /* enabled */);
  waitForEvent(CHRE_EVENT_BLE_SETTING_ENABLED);
  ASSERT_TRUE(
      EventLoopManagerSingleton::get()->getSettingManager().getSettingEnabled(
          Setting::BLE_AVAILABLE));
  waitForEvent(CHRE_EVENT_BLE_ADVERTISEMENT);
  ASSERT_TRUE(chrePalIsBleEnabled());
}

/**
 * Test that a nanoapp receives a function disabled error if it attempts to
 * start a scan when the BLE setting is disabled.
 */
TEST_F(TestBase, BleSettingDisabledStartScanTest) {
  setExpectedEventCounts();
  EventLoopManagerSingleton::get()->getSettingManager().postSettingChange(
      Setting::BLE_AVAILABLE, false /* enable */);
  loadNanoapp("Test nanoapp", gAppId, gAppVersion, gAppPerms, startBleNanoapp,
              handleEvent, endBleNanoapp);
  waitForEvent(CHRE_EVENT_SIMULATION_TEST_NANOAPP_LOADED);
  waitForEvent(CHRE_EVENT_BLE_FUNCTION_DISABLED_ERROR);
}

/**
 * Test that a nanoapp receives a success response when it attempts to stop a
 * BLE scan while the BLE setting is disabled.
 */
TEST_F(TestBase, BleSettingDisabledStopScanTest) {
  setExpectedEventCounts(0 /* start */, 0 /* advertisement */, 1 /* stop */);
  EventLoopManagerSingleton::get()->getSettingManager().postSettingChange(
      Setting::BLE_AVAILABLE, false /* enable */);
  loadNanoapp("Test nanoapp", gAppId, gAppVersion, gAppPerms, startBleNanoapp,
              handleEvent, endBleNanoapp);
  waitForEvent(CHRE_EVENT_SIMULATION_TEST_NANOAPP_LOADED);
  waitForEvent(CHRE_EVENT_BLE_STOP_SCAN);
}

}  // namespace chre
