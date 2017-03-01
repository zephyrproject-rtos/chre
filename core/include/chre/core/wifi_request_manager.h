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

#ifndef CHRE_CORE_WIFI_REQUEST_MANAGER_H_
#define CHRE_CORE_WIFI_REQUEST_MANAGER_H_

#include "chre/platform/platform_wifi.h"
#include "chre/util/non_copyable.h"

namespace chre {

/**
 * The WifiRequestManager handles requests from nanoapps for Wifi information.
 * This includes multiplexing multiple requests into one for the platform to
 * handle.
 *
 * This class is effectively a singleton as there can only be one entity of the
 * PlatformWifi instance.
 */
class WifiRequestManager : public NonCopyable {
 public:
  /**
   * @return the WiFi capabilities exposed by this platform.
   */
  uint32_t getCapabilities();

  /**
   * Handles the result of a request to PlatformWifi to change the state of the
   * scan monitor.
   *
   * @param enabled true if the result of the operation was an enabled scan
   *        monitor.
   * @param errorCode an error code that is provided to indicate success or what
   *        type of error has occurred. See the chreError enum in the CHRE API
   *        for additional details.
   */
  void handleScanMonitorStateChange(bool enabled, uint8_t errorCode);

  /**
   * Handles the result of a request to the PlatformWifi to request an active
   * Wifi scan.
   *
   * @param pending The result of the request was successful and the results
   *        be sent via the handleScanEvent method.
   * @param errorCode an error code that is used to indicate success or what
   *        type of error has occurred. See the chreError enum in the CHRE API
   *        for additional details.
   */
  void handleScanResponse(bool pending, uint8_t errorCode);

  /**
   * Handles a CHRE wifi scan event.
   *
   * @param event The wifi scan event provided to the wifi request manager. This
   *        memory is guaranteed not to be modified until it has been explicitly
   *        released through the PlatformWifi instance.
   */
  void handleScanEvent(chreWifiScanEvent *event);

 private:
  //! The instance of the platform wifi interface.
  PlatformWifi mPlatformWifi;
};

}  // namespace chre

#endif  // CHRE_CORE_WIFI_REQUEST_MANAGER_H_
