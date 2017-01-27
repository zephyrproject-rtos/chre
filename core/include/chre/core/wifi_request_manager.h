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

#include "chre/pal/wifi.h"
#include "chre/util/non_copyable.h"

namespace chre {

/**
 * The WifiRequestManager handles requests from nanoapps for Wifi information.
 * This includes multiplexing multiple requests into one for the platform to
 * handle.
 *
 * This class is effectively a singleton as there can only be one entity
 * controlling the chre wifi PAL.
 */
class WifiRequestManager : public NonCopyable {
 public:
  /**
   * Performs initiailization of the WifiRequestManager by obtaining the
   * instance of the PAL API and invoking open if the API is available on this
   * platform.
   */
  WifiRequestManager();

  /**
   * Performs deinitialization of the WifiRequestManager by closing down the
   * Wifi PAL.
   */
  ~WifiRequestManager();

  /**
   * Returns the set of WiFi capabilities that the platform has exposed. This
   * implementation maps directly into the PAL or returns
   * CHRE_WIFI_CAPABILITIES_NONE if the PAL was not loaded.
   *
   * @return the WiFi capabilities exposed by this platform.
   */
  uint32_t getCapabilities() const;

 private:
  //! The instance of the CHRE PAL API. This will be set to nullptr if the
  //! platform does not supply an implementation.
  const chrePalWifiApi *mWifiApi;

  //! Event handlers for the CHRE WiFi PAL. Refer to chre/pal/wifi.h for futher
  //! information.
  static void scanMonitorStatusChangeCallback(bool enabled, uint8_t errorCode);
  static void scanResponseCallback(bool pending, uint8_t errorCode);
  static void scanEventCallback(struct chreWifiScanEvent *event);
};

}  // namespace chre

#endif  // CHRE_CORE_WIFI_REQUEST_MANAGER_H_
