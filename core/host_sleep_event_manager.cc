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

#include "chre/core/host_sleep_event_manager.h"

#include "chre/core/event_loop_manager.h"
#include "chre/platform/assert.h"
#include "chre/platform/fatal_error.h"

namespace chre {

void HostSleepEventManager::handleHostSleep() {
  bool eventPosted = EventLoopManagerSingleton::get()->getEventLoop()
      .postEvent(CHRE_EVENT_HOST_ASLEEP, nullptr, nullptr);
  if (!eventPosted) {
    FATAL_ERROR("Failed to post sleep event");
  }
}

void HostSleepEventManager::handleHostAwake() {
  bool eventPosted = EventLoopManagerSingleton::get()->getEventLoop()
      .postEvent(CHRE_EVENT_HOST_AWAKE, nullptr, nullptr);
  if (!eventPosted) {
    FATAL_ERROR("Failed to post awake event");
  }
}

}  // namespace chre
