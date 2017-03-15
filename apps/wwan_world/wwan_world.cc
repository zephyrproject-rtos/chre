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

#include "chre.h"

#include <cinttypes>

namespace chre {
namespace app {

bool wwanWorldStart() {
  chreLog(CHRE_LOG_INFO, "WWAN world app started as instance %" PRIu32,
      chreGetInstanceId());

  const char *wwanCapabilitiesStr;
  uint32_t wwanCapabilities = chreWwanGetCapabilities();
  switch (wwanCapabilities) {
    case CHRE_WWAN_GET_CELL_INFO:
      wwanCapabilitiesStr = "GET_CELL_INFO";
      break;
    case CHRE_WWAN_CAPABILITIES_NONE:
      wwanCapabilitiesStr = "NONE";
      break;
    default:
      wwanCapabilitiesStr = "INVALID";
  }

  chreLog(CHRE_LOG_INFO, "Detected WWAN support as: %s (%" PRIu32 ")",
          wwanCapabilitiesStr, wwanCapabilities);
  return true;
}

void wwanWorldHandleEvent(uint32_t senderInstanceId,
                          uint16_t eventType,
                          const void *eventData) {
  // TODO: Implement this.
}

void wwanWorldStop() {
  chreLog(CHRE_LOG_INFO, "WWAN world app stopped");
}

}  // namespace app
}  // namespace chre
