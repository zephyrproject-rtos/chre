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

#ifndef CHRE_PLATFORM_PLATFORM_BLE_H_
#define CHRE_PLATFORM_PLATFORM_BLE_H_

#include "chre/target_platform/platform_ble_base.h"
#include "chre/util/time.h"

namespace chre {

class PlatformBle : public PlatformBleBase {
 public:
  /**
   * Performs platform-specific deinitialization of the PlatformBle instance.
   */
  ~PlatformBle();

  /**
   * Initializes the platform-specific BLE implementation. This is potentially
   * called at a later stage of initialization than the constructor, so platform
   * implementations are encouraged to put any blocking initialization here.
   */
  void init();

  /**
   * Returns the set of BLE capabilities that the platform has exposed. This
   * may return CHRE_BLE_CAPABILITIES_NONE if BLE is not supported.
   *
   * @return the BLE capabilities exposed by this platform.
   */
  uint32_t getCapabilities();

  /**
   * Returns the set of BLE filter capabilities that the platform has exposed.
   * This may return CHRE_BLE_FILTER_CAPABILITIES_NONE if BLE filtering is not
   * supported.
   *
   * @return the BLE filter capabilities exposed by this platform.
   */
  uint32_t getFilterCapabilities();
};

}  // namespace chre

#endif  // CHRE_PLATFORM_PLATFORM_BLE_H_
