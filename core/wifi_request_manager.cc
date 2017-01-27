/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <cinttypes>

#include "chre/core/wifi_request_manager.h"
#include "chre/platform/log.h"

namespace chre {

WifiRequestManager::WifiRequestManager() {
  mWifiApi = chrePalWifiGetApi(CHRE_PAL_WIFI_API_CURRENT_VERSION);
  if (mWifiApi != nullptr) {
    chrePalWifiCallbacks wifiCallbacks;
    wifiCallbacks.scanMonitorStatusChangeCallback =
        WifiRequestManager::scanMonitorStatusChangeCallback;
    wifiCallbacks.scanResponseCallback =
        WifiRequestManager::scanResponseCallback;
    wifiCallbacks.scanEventCallback =
        WifiRequestManager::scanEventCallback;
    if (!mWifiApi->open(&wifiCallbacks)) {
      LOGE("WiFi PAL open returned false");
      mWifiApi = nullptr;
    }
  } else {
    LOGW("Requested Wifi PAL (version %08" PRIx32 ") not found",
         CHRE_PAL_WIFI_API_CURRENT_VERSION);
  }
}

WifiRequestManager::~WifiRequestManager() {
  if (mWifiApi != nullptr) {
    mWifiApi->close();
  }
}

uint32_t WifiRequestManager::getCapabilities() const {
  if (mWifiApi != nullptr) {
    return mWifiApi->getCapabilities();
  } else {
    return CHRE_WIFI_CAPABILITIES_NONE;
  }
}

void WifiRequestManager::scanMonitorStatusChangeCallback(bool enabled,
                                                         uint8_t errorCode) {
  // TODO: Implement this.
}

void WifiRequestManager::scanResponseCallback(bool pending,
                                              uint8_t errorCode) {
  // TODO: Implement this.
}

void WifiRequestManager::scanEventCallback(struct chreWifiScanEvent *event) {
  // TODO: Implement this.
}

}  // namespace chre
