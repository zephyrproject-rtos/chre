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

#ifndef WIFI_SCAN_RESULT_H_
#define WIFI_SCAN_RESULT_H_

#include <cinttypes>

#include <chre.h>
#include "chre_cross_validation_wifi.nanopb.h"

class WifiScanResult {
 public:
  WifiScanResult() {
  }  // Need for default ctor of WifiScanResult arrays in manager

  /**
   * The ctor for the scan result for the AP.
   *
   * @param apScanResult The wifi scan result proto received from AP.
   */
  WifiScanResult(const chre_cross_validation_wifi_WifiScanResult &apScanResult);

  /**
   * The ctor for the scan result for CHRE.
   *
   * @param apScanResult The wifi scan result struct received from CHRE apis.
   */
  WifiScanResult(const chreWifiScanResult &chreScanResult);

  static bool areEqual(WifiScanResult result1, WifiScanResult result2);
};

#endif  // WIFI_SCAN_RESULT_H_
