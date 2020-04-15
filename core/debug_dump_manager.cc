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

#include "chre/core/debug_dump_manager.h"

#include "chre/core/event_loop_manager.h"
#include "chre/core/settings.h"

namespace chre {

void DebugDumpManager::trigger() {
  auto callback = [](uint16_t /* eventType */, void * /*eventData*/) {
    DebugDumpManager &debugDumpManager =
        EventLoopManagerSingleton::get()->getDebugDumpManager();
    debugDumpManager.collectFrameworkDebugDumps();
    debugDumpManager.sendFrameworkDebugDumps();
  };

  // Collect CHRE framework debug dumps.
  EventLoopManagerSingleton::get()->deferCallback(
      SystemCallbackType::PerformDebugDump, nullptr /*data*/, callback);
}

void DebugDumpManager::collectFrameworkDebugDumps() {
  auto *eventLoopManager = EventLoopManagerSingleton::get();

  eventLoopManager->getMemoryManager().logStateToBuffer(mDebugDump);
  eventLoopManager->getEventLoop().handleNanoappWakeupBuckets();
  eventLoopManager->getEventLoop().logStateToBuffer(mDebugDump);
  eventLoopManager->getSensorRequestManager().logStateToBuffer(mDebugDump);
#ifdef CHRE_GNSS_SUPPORT_ENABLED
  eventLoopManager->getGnssManager().logStateToBuffer(mDebugDump);
#endif  // CHRE_GNSS_SUPPORT_ENABLED
#ifdef CHRE_WIFI_SUPPORT_ENABLED
  eventLoopManager->getWifiRequestManager().logStateToBuffer(mDebugDump);
#endif  // CHRE_WIFI_SUPPORT_ENABLED
#ifdef CHRE_WWAN_SUPPORT_ENABLED
  eventLoopManager->getWwanRequestManager().logStateToBuffer(mDebugDump);
#endif  // CHRE_WWAN_SUPPORT_ENABLED
#ifdef CHRE_AUDIO_SUPPORT_ENABLED
  eventLoopManager->getAudioRequestManager().logStateToBuffer(mDebugDump);
#endif  // CHRE_AUDIO_SUPPORT_ENABLED
  logSettingStateToBuffer(mDebugDump);
}

void DebugDumpManager::sendFrameworkDebugDumps() {
  for (size_t i = 0; i < mDebugDump.getBuffers().size() - 1; i++) {
    const auto &buff = mDebugDump.getBuffers()[i];
    sendDebugDump(buff.get(), false /*complete*/);
  }
  sendDebugDump(mDebugDump.getBuffers().back().get(), true /*complete*/);

  // Clear current session debug dumps and release memory.
  mDebugDump.clear();
}

}  // namespace chre
