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

#ifndef CHRE_CORE_HOST_SLEEP_EVENT_MANAGER_H_
#define CHRE_CORE_HOST_SLEEP_EVENT_MANAGER_H_

#include "chre/core/nanoapp.h"
#include "chre/util/non_copyable.h"

namespace chre {

/**
 * Handles notifications that the CHRE host has gone to sleep or is awake.
 */
class HostSleepEventManager : public NonCopyable {
 public:
  /**
   * Invoked by the platform to notify CHRE that the host has gone to sleep.
   */
  void handleHostSleep();

  /**
   * Invoked by the platform to notify CHRE that the host has become awake.
   */
  void handleHostAwake();

  /**
   * @return true if the host is awake, false otherwise.
   */
  bool hostIsAwake() {
    return mHostIsAwake;
  }

 private:
  // TODO(P1-d395b8): Remove this bool and pass the API call to the platform.
  //! Set to true if the host is awake, false if asleep.
  bool mHostIsAwake = true;
};

}  // namespace chre

#endif  // CHRE_CORE_HOST_SLEEP_EVENT_MANAGER_H_
