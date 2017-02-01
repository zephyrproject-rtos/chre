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

#ifndef CHRE_CORE_WIFI_SCAN_REQUEST_H_
#define CHRE_CORE_WIFI_SCAN_REQUEST_H_

#include "chre_api/chre/wifi.h"

namespace chre {

/**
 * This WifiScanType is designed to wrap constants provided by the CHRE API to
 * improve type-safety. In addition, an invalid wifi scan type is added for
 * handling an app that is not requesting wifi scans.
 */
enum class WifiScanType {
  Invalid,
  Active,
  ActivePlusPassiveDfs,
  Passive
};

/**
 * Translates a CHRE API enum wifi scan type to a WifiScanType. This funciton
 * also performs input validation and will default to WifiScanType::Invalid if
 * the provided value is not a valid enumeration.
 *
 * @param enumWifiScanType A potentially unsafe CHRE API enum wifi scan type.
 * @return a WifiScanType given a CHRE API wifi scan type.
 */
WifiScanType getWifiScanTypeForEnum(enum chreWifiScanType enumWifiScanType);

/**
 * Models a request for wifi scans. This class implements the API set forth by
 * the RequestMultiplexer container in addition to specific functionality
 * required for requesting wifi scans.
 */
class WifiScanRequest {
 public:
  /**
   * Default constructs a wifi scan request to the minimal possible
   * configuration. The WifiScanType is set to Invalid and the frequency and
   * SSID lists are both cleared.
   */
  WifiScanRequest();

  /**
   * TODO: Write a doxygen comment once this class is complete.
   */
  WifiScanRequest(WifiScanType wifiScanType);

  /**
   * @return the type of this scan request.
   */
  WifiScanType getScanType() const;

  // TODO: Implement the remaining methods required for the RequestMultiplexer
  // container.

 private:
  //! The type of request for this scan.
  WifiScanType mScanType;
};

}  // namespace chre

#endif  // CHRE_CORE_WIFI_SCAN_REQUEST_H_
