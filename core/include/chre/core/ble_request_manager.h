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

#ifndef CHRE_CORE_BLE_REQUEST_MANAGER_H_
#define CHRE_CORE_BLE_REQUEST_MANAGER_H_

#include <cstdint>

#include "chre/platform/platform_ble.h"
#include "chre/util/non_copyable.h"

namespace chre {

/**
 * Manages requests for ble resources from nanoapps and multiplexes these
 * requests into the platform-specific implementation of the ble subsystem.
 */
class BleRequestManager : public NonCopyable {
 public:
  /**
   * Initializes the underlying platform-specific BLE module. Must be called
   * prior to invoking any other methods in this class.
   */
  void init();

  /**
   * @return the BLE capabilities exposed by this platform.
   */
  uint32_t getCapabilities();

  /**
   * @return the BLE filter capabilities exposed by this platform.
   */
  uint32_t getFilterCapabilities();

 private:
  //! The platform BLE interface.
  PlatformBle mPlatformBle;
};

}  // namespace chre

#endif  // CHRE_CORE_BLE_REQUEST_MANAGER_H_
