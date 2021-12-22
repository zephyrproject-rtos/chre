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

#include "chre/pal/ble.h"

/**
 * A simulated implementation of the BLE PAL for the linux platform.
 */
namespace {
const struct chrePalSystemApi *gSystemApi = nullptr;
const struct chrePalBleCallbacks *gCallbacks = nullptr;

uint32_t chrePalBleGetCapabilities() {
  return CHRE_BLE_CAPABILITIES_SCAN |
         CHRE_BLE_CAPABILITIES_SCAN_RESULT_BATCHING |
         CHRE_BLE_CAPABILITIES_SCAN_FILTER_BEST_EFFORT;
}

uint32_t chrePalBleGetFilterCapabilities() {
  return CHRE_BLE_FILTER_CAPABILITIES_RSSI |
         CHRE_BLE_FILTER_CAPABILITIES_SERVICE_DATA_UUID;
}

bool chrePalBleStartScan(chreBleScanMode /* mode */,
                         uint32_t /* reportDelayMs */,
                         const struct chreBleScanFilter * /* filter */) {
  // TODO(b/211899620): Implement method.
  return false;
}

bool chrePalBleStopScan() {
  // TODO(b/211899620): Implement method.
  return false;
}

void chrePalBleReleaseAdvertisingEvent(
    struct chreBleAdvertisementEvent * /* event */) {
  // TODO(b/211899620): Implement method.
}

void chrePalBleApiClose() {}

bool chrePalBleApiOpen(const struct chrePalSystemApi *systemApi,
                       const struct chrePalBleCallbacks *callbacks) {
  chrePalBleApiClose();

  bool success = false;
  if (systemApi != nullptr && callbacks != nullptr) {
    gSystemApi = systemApi;
    gCallbacks = callbacks;
    success = true;
  }

  return success;
}

}  // anonymous namespace

const struct chrePalBleApi *chrePalBleGetApi(uint32_t requestedApiVersion) {
  static const struct chrePalBleApi kApi = {
      .moduleVersion = CHRE_PAL_BLE_API_CURRENT_VERSION,
      .open = chrePalBleApiOpen,
      .close = chrePalBleApiClose,
      .getCapabilities = chrePalBleGetCapabilities,
      .getFilterCapabilities = chrePalBleGetFilterCapabilities,
      .startScan = chrePalBleStartScan,
      .stopScan = chrePalBleStopScan,
      .releaseAdvertisingEvent = chrePalBleReleaseAdvertisingEvent,
  };

  if (!CHRE_PAL_VERSIONS_ARE_COMPATIBLE(kApi.moduleVersion,
                                        requestedApiVersion)) {
    return nullptr;
  } else {
    return &kApi;
  }
}
