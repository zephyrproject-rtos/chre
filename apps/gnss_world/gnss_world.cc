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

bool gnssWorldStart() {
  chreLog(CHRE_LOG_INFO, "GNSS world app started as instance %" PRIu32,
      chreGetInstanceId());

  const char *gnssCapabilitiesStr;
  uint32_t gnssCapabilities = chreGnssGetCapabilities();
  switch (gnssCapabilities) {
    case CHRE_GNSS_CAPABILITIES_LOCATION
        | CHRE_GNSS_CAPABILITIES_MEASUREMENTS:
      gnssCapabilitiesStr = "LOCATION | MEASUREMENTS";
      break;
    case CHRE_GNSS_CAPABILITIES_LOCATION:
      gnssCapabilitiesStr = "LOCATION";
      break;
    case CHRE_GNSS_CAPABILITIES_MEASUREMENTS:
      gnssCapabilitiesStr = "MEASUREMENTS";
      break;
    case CHRE_GNSS_CAPABILITIES_NONE:
      gnssCapabilitiesStr = "NONE";
      break;
    default:
      gnssCapabilitiesStr = "INVALID";
  }

  chreLog(CHRE_LOG_INFO, "Detected GNSS support as: %s (%" PRIu32 ")",
          gnssCapabilitiesStr, gnssCapabilities);
  return true;
}

void gnssWorldHandleEvent(uint32_t senderInstanceId,
                          uint16_t eventType,
                          const void *eventData) {
  // TODO: Implement this.
}

void gnssWorldStop() {
  chreLog(CHRE_LOG_INFO, "GNSS world app stopped");
}

}  // namespace app
}  // namespace chre
