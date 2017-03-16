/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <chre.h>
#include <cinttypes>

#include "chre/util/nanoapp/log.h"

#define LOG_TAG "[WifiWorld]"

namespace chre {
namespace app {

//! A dummy cookie to pass into the configure scan monitoring async request.
const uint32_t kScanMonitoringCookie = 0x1337;

namespace {

/**
 * Handles the result of an asynchronous request for a wifi resource.
 *
 * @param result a pointer to the event structure containing the result of the
 * request.
 */
void handleWifiAsyncResult(const chreAsyncResult *result) {
  if (result->requestType == CHRE_WIFI_REQUEST_TYPE_CONFIGURE_SCAN_MONITOR) {
    if (result->success) {
      LOGI("Successfully requested wifi scan monitoring");
    } else {
      LOGI("Error requesting wifi scan monitoring with %" PRIu8,
           result->errorCode);
    }

    if (result->cookie != &kScanMonitoringCookie) {
      LOGE("Scan monitoring request cookie mismatch");
    }
  }
}

/**
 * Handles a wifi scan event.
 *
 * @param event a pointer to the details of the wifi scan event.
 */
void handleWifiScanEvent(const chreWifiScanEvent *event) {
  // TODO: Log the scan result.
}

}  // namespace

bool wifiWorldStart() {
  LOGI("App started as instance %" PRIu32, chreGetInstanceId());

  const char *wifiCapabilitiesStr;
  uint32_t wifiCapabilities = chreWifiGetCapabilities();
  switch (wifiCapabilities) {
    case CHRE_WIFI_CAPABILITIES_ON_DEMAND_SCAN
        | CHRE_WIFI_CAPABILITIES_SCAN_MONITORING:
      wifiCapabilitiesStr = "ON_DEMAND_SCAN | SCAN_MONITORING";
      break;
    case CHRE_WIFI_CAPABILITIES_ON_DEMAND_SCAN:
      wifiCapabilitiesStr = "ON_DEMAND_SCAN";
      break;
    case CHRE_WIFI_CAPABILITIES_SCAN_MONITORING:
      wifiCapabilitiesStr = "SCAN_MONITORING";
      break;
    case CHRE_WIFI_CAPABILITIES_NONE:
      wifiCapabilitiesStr = "NONE";
      break;
    default:
      wifiCapabilitiesStr = "INVALID";
  }

  LOGI("Detected WiFi support as: %s (%" PRIu32 ")",
       wifiCapabilitiesStr, wifiCapabilities);

  if (wifiCapabilities & CHRE_WIFI_CAPABILITIES_SCAN_MONITORING) {
    if (chreWifiConfigureScanMonitorAsync(true, &kScanMonitoringCookie)) {
      LOGI("Scan monitor enable request successful");
    } else {
      LOGE("Error sending scan monitoring request");
    }
  }

  return true;
}

void wifiWorldHandleEvent(uint32_t senderInstanceId,
                          uint16_t eventType,
                          const void *eventData) {
  switch (eventType) {
    case CHRE_EVENT_WIFI_ASYNC_RESULT:
      handleWifiAsyncResult(static_cast<const chreAsyncResult *>(eventData));
      break;
    case CHRE_EVENT_WIFI_SCAN_RESULT:
      handleWifiScanEvent(static_cast<const chreWifiScanEvent *>(eventData));
      break;
    default:
      LOGW("Unhandled event type %" PRIu16, eventType);
  }
}

void wifiWorldStop() {
  LOGI("Wifi world app stopped");
}

}  // namespace app
}  // namespace chre
