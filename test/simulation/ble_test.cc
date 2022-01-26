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
#include "chre/platform/fatal_error.h"
#include "chre/util/dynamic_vector.h"
#include "chre_api/chre/ble.h"
#include "test_util.h"

namespace chre {

#define CHRE_EVENT_BLE_START_SCAN CHRE_SPECIFIC_SIMULATION_TEST_EVENT_ID(0)
#define CHRE_EVENT_BLE_STOP_SCAN CHRE_SPECIFIC_SIMULATION_TEST_EVENT_ID(1)

namespace {

uint64_t gAppId = 0x0123456789abcdef;
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
  uint16_t testType = eventType;
  if (eventType == CHRE_EVENT_BLE_ASYNC_RESULT) {
    const struct chreAsyncResult *data =
        static_cast<const struct chreAsyncResult *>(eventData);
    testType = (data->requestType == CHRE_BLE_REQUEST_TYPE_START_SCAN)
                   ? CHRE_EVENT_BLE_START_SCAN
                   : CHRE_EVENT_BLE_STOP_SCAN;
  }
  return testType;
}

struct EventCounts {
  uint8_t start;
  uint8_t stop;
  uint8_t advertisment;
};

EventCounts gExpectedCounts{0, 0, 0};
EventCounts gCounts{0, 0, 0};

void setCountsForTest(uint8_t start, uint8_t advertisement, uint8_t stop) {
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
  setCountsForTest(start, advertisement, stop);
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
  } else {
    FATAL_ERROR("Nanoapp should not receive a non BLE event: 0x%" PRIx16,
                testEvent);
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
  return true;
}

void endBleNanoapp() {
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

}  // namespace chre
