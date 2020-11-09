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

#include "chre/pal/util/wifi_scan_cache.h"

#include "chre/util/macros.h"

bool chreWifiScanCacheInit(const struct chrePalSystemApi *systemApi,
                           const struct chrePalWifiCallbacks *callbacks) {
  UNUSED_VAR(systemApi);
  UNUSED_VAR(callbacks);
  // TODO(arthuri): Implement this
  return false;
}

void chreWifiScanCacheDeinit(void) {
  // TODO(arthuri): Implement this
}

void chreWifiScanCacheScanEventBegin(enum chreWifiScanType scanType,
                                     uint8_t ssidSetSize,
                                     const uint32_t *scannedFreqList,
                                     uint16_t scannedFreqListLength,
                                     uint8_t radioChainPref,
                                     bool activeScanResult) {
  UNUSED_VAR(scanType);
  UNUSED_VAR(ssidSetSize);
  UNUSED_VAR(scannedFreqList);
  UNUSED_VAR(scannedFreqListLength);
  UNUSED_VAR(radioChainPref);
  UNUSED_VAR(activeScanResult);
  // TODO(arthuri): Implement this
}

void chreWifiScanCacheScanEventAdd(const struct chreWifiScanResult *result) {
  UNUSED_VAR(result);
  // TODO(arthuri): Implement this
}

void chreWifiScanCacheScanEventEnd(enum chreError errorCode) {
  UNUSED_VAR(errorCode);
  // TODO(arthuri): Implement this
}

bool chreWifiScanCacheDispatchFromCache(
    const struct chreWifiScanParams *params) {
  UNUSED_VAR(params);
  // TODO(arthuri): Implement this
  return false;
}

void chreWifiScanCacheReleaseScanEvent(struct chreWifiScanEvent *event) {
  UNUSED_VAR(event);
  // TODO(arthuri): Implement this
}

void chreWifiScanCacheConfigureScanMonitor(bool enable) {
  UNUSED_VAR(enable);
  // TODO(arthuri): Implement this
}
