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

#include "chre/util/unique_ptr.h"

#include <future>
#include <thread>

/**
 * A simulated implementation of the BLE PAL for the linux platform.
 */
namespace {
const struct chrePalSystemApi *gSystemApi = nullptr;
const struct chrePalBleCallbacks *gCallbacks = nullptr;

std::thread gBleStartScanThread;
std::thread gBleStopScanThread;
std::promise<void> gStopAdvertisingEvents;
std::chrono::milliseconds gMinIntervalMs(50);

bool gBleEnabled = false;

void startScan() {
  gCallbacks->scanStatusChangeCallback(true, CHRE_ERROR_NONE);
  std::future<void> signal = gStopAdvertisingEvents.get_future();
  while (signal.wait_for(gMinIntervalMs) == std::future_status::timeout) {
    auto event = chre::MakeUniqueZeroFill<struct chreBleAdvertisementEvent>();
    auto report = chre::MakeUniqueZeroFill<struct chreBleAdvertisingReport>();
    event->reports = report.release();
    event->numReports = 1;
    gCallbacks->advertisingEventCallback(event.release());
  }
}

void stopScan() {
  gCallbacks->scanStatusChangeCallback(false, CHRE_ERROR_NONE);
}

void stopThreads() {
  if (gBleStartScanThread.joinable()) {
    gStopAdvertisingEvents.set_value();
    gBleStartScanThread.join();
  }
  if (gBleStopScanThread.joinable()) {
    gBleStopScanThread.join();
  }
}

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
  stopThreads();
  gStopAdvertisingEvents = std::promise<void>();
  gBleStartScanThread = std::thread(startScan);
  gBleEnabled = true;
  return true;
}

bool chrePalBleStopScan() {
  stopThreads();
  gBleStopScanThread = std::thread(stopScan);
  gBleEnabled = false;
  return true;
}

void chrePalBleReleaseAdvertisingEvent(
    struct chreBleAdvertisementEvent *event) {
  for (size_t i = 0; i < event->numReports; i++) {
    chre::memoryFree(
        const_cast<chreBleAdvertisingReport *>(&(event->reports[i])));
  }
  chre::memoryFree(event);
}

void chrePalBleApiClose() {
  stopThreads();
}

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

bool chrePalIsBleEnabled() {
  return gBleEnabled;
}

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
