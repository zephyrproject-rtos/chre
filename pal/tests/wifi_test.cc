/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "chre/pal/wifi.h"
#include "chre/platform/condition_variable.h"
#include "chre/platform/log.h"
#include "chre/platform/mutex.h"
#include "chre/platform/system_time.h"
#include "chre/util/dynamic_vector.h"
#include "chre/util/lock_guard.h"
#include "chre/util/nanoapp/wifi.h"
#include "chre/util/time.h"
#include "gtest/gtest.h"

#include <cinttypes>

namespace {

using ::chre::Nanoseconds;
using ::chre::Seconds;
using ::chre::SystemTime;

uint8_t gErrorCode = CHRE_ERROR_LAST;
uint32_t gNumScanEvents = 0;
bool gLastScanEventReceived = false;

//! Mutex to protect global variables
chre::Mutex gMutex;

chre::ConditionVariable gCondVar;

void logChreWifiResult(const chreWifiScanResult &result) {
  const char *ssidStr = "<non-printable>";
  char ssidBuffer[chre::kMaxSsidStrLen];
  if (result.ssidLen == 0) {
    ssidStr = "<empty>";
  } else if (chre::parseSsidToStr(ssidBuffer, sizeof(ssidBuffer), result.ssid,
                                  result.ssidLen)) {
    ssidStr = ssidBuffer;
  }

  LOGI("Found network with SSID: %s", ssidStr);
  const char *bssidStr = "<non-printable>";
  char bssidBuffer[chre::kBssidStrLen];
  if (chre::parseBssidToStr(result.bssid, bssidBuffer, sizeof(bssidBuffer))) {
    bssidStr = bssidBuffer;
  }

  LOGI("  age (ms): %" PRIu32, result.ageMs);
  LOGI("  capability info: 0x%" PRIx16, result.capabilityInfo);
  LOGI("  bssid: %s", bssidStr);
  LOGI("  flags: 0x%" PRIx8, result.flags);
  LOGI("  rssi: %" PRId8 "dBm", result.rssi);
  LOGI("  band: %s (%" PRIu8 ")", chre::parseChreWifiBand(result.band),
       result.band);
  LOGI("  primary channel: %" PRIu32, result.primaryChannel);
  LOGI("  center frequency primary: %" PRIu32, result.centerFreqPrimary);
  LOGI("  center frequency secondary: %" PRIu32, result.centerFreqSecondary);
  LOGI("  channel width: %" PRIu8, result.channelWidth);
  LOGI("  security mode: 0x%" PRIx8, result.securityMode);
}

void chrePalScanMonitorStatusChangeCallback(bool enabled, uint8_t errorCode) {
  // TODO:
}

void chrePalScanResponseCallback(bool pending, uint8_t errorCode) {
  LOGI("Received scan response with pending %d error %" PRIu8, pending,
       errorCode);
  chre::LockGuard<chre::Mutex> lock(gMutex);
  gErrorCode = errorCode;

  // TODO: Verify async result is received within required
  // CHRE_WIFI_SCAN_RESULT_TIMEOUT_NS timeout
}

void chrePalScanEventCallback(struct chreWifiScanEvent *event) {
  if (event == nullptr) {
    LOGE("Got null scan event");
  } else {
    // TODO: Sanity check values, push values onto a vector
    // so that validation can occur on the main test thread
    for (uint8_t i = 0; i < event->resultCount; i++) {
      const chreWifiScanResult &result = event->results[i];
      logChreWifiResult(result);
    }

    {
      chre::LockGuard<chre::Mutex> lock(gMutex);
      gNumScanEvents += event->resultCount;
      gLastScanEventReceived = (gNumScanEvents == event->resultTotal);
    }

    gCondVar.notify_one();
  }
}

void chrePalRangingEventCallback(uint8_t errorCode,
                                 struct chreWifiRangingEvent *event) {
  // TODO:
}
}  // anonymous namespace

TEST(PalWifiTest, ScanAsyncTest) {
  const struct chrePalWifiApi *api =
      chrePalWifiGetApi(CHRE_PAL_WIFI_API_CURRENT_VERSION);
  ASSERT_NE(api, nullptr);
  EXPECT_EQ(api->moduleVersion, CHRE_PAL_WIFI_API_CURRENT_VERSION);

  // Open the PAL API
  static const struct chrePalWifiCallbacks kCallbacks = {
      .scanMonitorStatusChangeCallback = chrePalScanMonitorStatusChangeCallback,
      .scanResponseCallback = chrePalScanResponseCallback,
      .scanEventCallback = chrePalScanEventCallback,
      .rangingEventCallback = chrePalRangingEventCallback,
  };
  // TODO: Supply systemApi
  ASSERT_TRUE(api->open(nullptr /* systemApi */, &kCallbacks));

  // Request a WiFi scan
  chre::LockGuard<chre::Mutex> lock(gMutex);
  gErrorCode = CHRE_ERROR_LAST;
  gNumScanEvents = 0;
  gLastScanEventReceived = false;

  struct chreWifiScanParams params = {};
  params.scanType = CHRE_WIFI_SCAN_TYPE_ACTIVE;
  params.maxScanAgeMs = 5000;  // 5 seconds
  params.frequencyListLen = 0;
  params.ssidListLen = 0;
  params.radioChainPref = CHRE_WIFI_RADIO_CHAIN_PREF_DEFAULT;
  // TODO: Change below to ASSERT_TRUE, and modify test so that
  // api->close() is always called.
  EXPECT_TRUE(api->requestScan(&params));

  // Since the CHRE API only poses timeout requirements on the async response,
  // place a timeout longer than the CHRE_WIFI_SCAN_RESULT_TIMEOUT_NS.
  const Nanoseconds kTimeoutNs = Nanoseconds(Seconds(60));
  Nanoseconds end = SystemTime::getMonotonicTime() + kTimeoutNs;
  while (!gLastScanEventReceived && SystemTime::getMonotonicTime() < end) {
    gCondVar.wait_for(gMutex, kTimeoutNs);
  }

  EXPECT_EQ(gErrorCode, CHRE_ERROR_NONE);
  EXPECT_TRUE(gLastScanEventReceived);
  EXPECT_GT(gNumScanEvents, 0);

  api->close();
}
