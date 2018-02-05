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

#include "chre/platform/power_control_manager.h"

#include "chre/platform/slpi/power_control_util.h"

namespace chre {

PowerControlManagerBase::PowerControlManagerBase() {
#ifdef CHRE_SLPI_UIMG_ENABLED
  // TODO(P2-51d645): Initialize PM client
#endif // CHRE_SLPI_UIMG_ENABLED
}

PowerControlManagerBase::~PowerControlManagerBase() {
#ifdef CHRE_SLPI_UIMG_ENABLED
  // TODO(P2-51d645): Release PM client
#endif // CHRE_SLPI_UIMG_ENABLED
}

bool PowerControlManagerBase::voteBigImage(bool bigImage) {
#ifdef CHRE_SLPI_UIMG_ENABLED
  // TODO(P2-51d645): Implement this
  #error SLPI/SEE voteBigImage not supported
#else
  return true;
#endif // CHRE_SLPI_UIMG_ENABLED
}

void PowerControlManagerBase::onHostWakeSuspendEvent(bool awake) {
  if (mHostIsAwake != awake) {
    mHostIsAwake = awake;

    EventLoopManagerSingleton::get()->getEventLoop().postEvent(
        mHostIsAwake ? CHRE_EVENT_HOST_AWAKE : CHRE_EVENT_HOST_ASLEEP,
        nullptr /* eventData */, nullptr /* freeCallback */);
  }
}

void PowerControlManager::postEventLoopProcess(size_t numPendingEvents) {
  if (numPendingEvents == 0 && !slpiInUImage()) {
    voteBigImage(false);
  }
}

bool PowerControlManager::hostIsAwake() {
  return mHostIsAwake;
}

} // namespace chre
