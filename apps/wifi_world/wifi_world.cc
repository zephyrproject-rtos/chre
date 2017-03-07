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

#include "chre.h"

#include <cinttypes>

#include "chre/platform/log.h"

namespace chre {
namespace app {

bool wifiWorldStart() {
  chreLog(CHRE_LOG_INFO, "Wifi world app started as instance %" PRIu32,
      chreGetInstanceId());

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

  chreLog(CHRE_LOG_INFO, "Detected WiFi support as: %s (%" PRIu32 ")",
          wifiCapabilitiesStr, wifiCapabilities);
  return true;
}

void wifiWorldHandleEvent(uint32_t senderInstanceId,
                          uint16_t eventType,
                          const void *eventData) {
}

void wifiWorldStop() {
  chreLog(CHRE_LOG_INFO, "Wifi world app stopped");
}

}  // namespace app
}  // namespace chre
