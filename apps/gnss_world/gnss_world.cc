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

#include <chre.h>
#include <cinttypes>

#include "chre/util/nanoapp/log.h"
#include "chre/util/time.h"

#define LOG_TAG "[GnssWorld]"

#ifdef CHRE_NANOAPP_INTERNAL
namespace chre {
namespace {
#endif  // CHRE_NANOAPP_INTERNAL

//! A dummy cookie to pass into the location session start async request.
const uint32_t kLocationSessionStartCookie = 0x1337;

//! The interval between location updates.
constexpr Milliseconds kLocationInterval(Seconds(10));

//! The minimum time to the next fix for a location.
constexpr Milliseconds kLocationMinTimeToNextFix(0);

void handleGnssAsyncResult(const chreAsyncResult *result) {
  if (result->requestType == CHRE_GNSS_REQUEST_TYPE_LOCATION_SESSION_START) {
    if (result->success) {
      LOGI("Successfully requested a GNSS location session");
    } else {
      LOGE("Error requesting GNSS scan monitoring with %" PRIu8,
           result->errorCode);
    }

    if (result->cookie != &kLocationSessionStartCookie) {
      LOGE("Location session start request cookie mismatch");
    }
  } else {
    LOGE("Received invalid async result");
  }
}

void handleGnssLocationEvent(const chreGnssLocationEvent *event) {
  LOGI("Received location: %" PRId32 ", %" PRId32, event->latitude_deg_e7,
       event->longitude_deg_e7);
  LOGI("  timestamp (ms): %" PRIu64, event->timestamp);
  LOGI("  altitude (m): %f", event->altitude);
  LOGI("  speed (m/s): %f", event->speed);
  LOGI("  bearing (deg): %f", event->bearing);
  LOGI("  accuracy: %f", event->accuracy);
  LOGI("  flags: %" PRIx16, event->flags);
}

bool nanoappStart() {
  LOGI("App started as instance %" PRIu32, chreGetInstanceId());

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

  LOGI("Detected GNSS support as: %s (%" PRIu32 ")",
       gnssCapabilitiesStr, gnssCapabilities);

  if (gnssCapabilities & CHRE_GNSS_CAPABILITIES_LOCATION) {
    if (chreGnssLocationSessionStartAsync(
        kLocationInterval.getMilliseconds(),
        kLocationMinTimeToNextFix.getMilliseconds(),
        &kLocationSessionStartCookie)) {
      LOGI("Location session start request sent");
    } else {
      LOGE("Error sending location session request");
    }
  }

  return true;
}

void nanoappHandleEvent(uint32_t senderInstanceId,
                        uint16_t eventType,
                        const void *eventData) {
  switch (eventType) {
    case CHRE_EVENT_GNSS_ASYNC_RESULT:
      handleGnssAsyncResult(static_cast<const chreAsyncResult *>(eventData));
      break;
    case CHRE_EVENT_GNSS_LOCATION:
      handleGnssLocationEvent(
          static_cast<const chreGnssLocationEvent *>(eventData));
      break;
    default:
      LOGW("Unhandled event type %" PRIu16, eventType);
  }
}

void nanoappEnd() {
  LOGI("Stopped");
}

#ifdef CHRE_NANOAPP_INTERNAL
}  // anonymous namespace
}  // namespace chre

#include "chre/util/nanoapp/app_id.h"
#include "chre/platform/static_nanoapp_init.h"

CHRE_STATIC_NANOAPP_INIT(GnssWorld, chre::kGnssWorldAppId, 0);
#endif  // CHRE_NANOAPP_INTERNAL
