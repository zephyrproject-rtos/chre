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

#ifndef CHRE_PLATFORM_SLPI_SEE_POWER_CONTROL_MANAGER_BASE_H_
#define CHRE_PLATFORM_SLPI_SEE_POWER_CONTROL_MANAGER_BASE_H_

extern "C" {

#include "sns_island_util.h"

} // extern "C"

namespace chre {

class PowerControlManagerBase {
 public:
  PowerControlManagerBase();
  ~PowerControlManagerBase();

  /**
   * Votes for a power mode to the SLPI power manager.
   *
   * @param bigImage Whether to vote for bigImage or not.
   *
   * @return true if the vote returned success.
   */
  bool voteBigImage(bool bigImage);

  /**
   * Sets the AP awake/suspended state and posts an event to interested
   * nanoapps. This method should only be invoked by the SEE helper as a
   * result of an event from the remote_proc_state sensor.
   *
   * @param awake true if the AP is awake, false otherwise
   */
  void onHostWakeSuspendEvent(bool awake);

 protected:
  //! Client handle for the island aggregator registration.
  sns_island_client_handle mClientHandle = nullptr;

  //! Set to true if the host is awake, false if suspended.
  bool mHostIsAwake = true;
};

} // namespace chre

#endif // CHRE_PLATFORM_SLPI_SEE_POWER_CONTROL_MANAGER_BASE_H_
