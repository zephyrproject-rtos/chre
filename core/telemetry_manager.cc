/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include "chre/core/telemetry_manager.h"

#include <pb_encode.h>

#include "chre/core/event_loop_manager.h"
#include "chre/platform/fatal_error.h"
#include "chre/platform/shared/host_protocol_chre.h"
#include "chre/util/macros.h"
#include "chre/util/nested_data_ptr.h"
#include "chre/util/time.h"
#include "pixelatoms.nanopb.h"

namespace chre {

namespace {

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!! DISCLAIMER !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// The metrics implemented in this class makes use of open-sourced PixelAtoms,
// but they are not Pixel-specific, and can be extended to OEM use. If you
// would like to use this code for telemetry purposes, please contact us for
// details.

//! Helper define macros for nanopb types.
#define PIXELATOMS_GET(x) android_hardware_google_pixel_PixelAtoms_##x
#define PIXELATOMS_GET_PAL_TYPE(x)                        \
  _android_hardware_google_pixel_PixelAtoms_ChrePalType:: \
      android_hardware_google_pixel_PixelAtoms_ChrePalType_CHRE_PAL_TYPE_##x

void sendPalOpenFailedMetric(
    _android_hardware_google_pixel_PixelAtoms_ChrePalType pal) {
  _android_hardware_google_pixel_PixelAtoms_ChrePalOpenFailed result =
      PIXELATOMS_GET(ChrePalOpenFailed_init_default);
  result.has_pal = true;
  result.pal = pal;
  result.has_type = true;
  result
      .type = _android_hardware_google_pixel_PixelAtoms_ChrePalOpenFailed_Type::
      android_hardware_google_pixel_PixelAtoms_ChrePalOpenFailed_Type_INITIAL_OPEN;

  size_t size;
  if (!pb_get_encoded_size(&size, PIXELATOMS_GET(ChrePalOpenFailed_fields),
                           &result)) {
    LOGE("Failed to get message size");
  } else {
    pb_byte_t *bytes = static_cast<pb_byte_t *>(memoryAlloc(size));
    if (bytes == nullptr) {
      LOG_OOM();
    } else {
      pb_ostream_t stream = pb_ostream_from_buffer(bytes, size);
      if (!pb_encode(&stream, PIXELATOMS_GET(ChrePalOpenFailed_fields),
                     &result)) {
        LOGE("Failed to metric error %s", PB_GET_ERROR(&stream));
      } else {
        HostCommsManager &manager =
            EventLoopManagerSingleton::get()->getHostCommsManager();
        if (!manager.sendMetricLog(
                PIXELATOMS_GET(Atom_chre_pal_open_failed_tag), bytes, size)) {
          LOGE("Failed to send PAL open failed metric message");
        }
      }
      memoryFree(bytes);
    }
  }
}

_android_hardware_google_pixel_PixelAtoms_ChrePalType toAtomPalType(
    TelemetryManager::PalType type) {
  switch (type) {
    case TelemetryManager::PalType::SENSOR:
      return PIXELATOMS_GET_PAL_TYPE(SENSOR);
    case TelemetryManager::PalType::WIFI:
      return PIXELATOMS_GET_PAL_TYPE(WIFI);
    case TelemetryManager::PalType::GNSS:
      return PIXELATOMS_GET_PAL_TYPE(GNSS);
    case TelemetryManager::PalType::WWAN:
      return PIXELATOMS_GET_PAL_TYPE(WWAN);
    case TelemetryManager::PalType::AUDIO:
      return PIXELATOMS_GET_PAL_TYPE(AUDIO);
    case TelemetryManager::PalType::BLE:
      return PIXELATOMS_GET_PAL_TYPE(BLE);
    case TelemetryManager::PalType::UNKNOWN:
    default:
      LOGW("Unknown PAL type %" PRIu8, type);
      return PIXELATOMS_GET_PAL_TYPE(UNKNOWN);
  }
}

}  // anonymous namespace

TelemetryManager::TelemetryManager() {
  scheduleMetricTimer();
}

void TelemetryManager::onPalOpenFailure(PalType type) {
  auto callback = [](uint16_t /*type*/, void *data, void * /*extraData*/) {
    _android_hardware_google_pixel_PixelAtoms_ChrePalType palType =
        toAtomPalType(NestedDataPtr<PalType>(data));

    if (palType != PIXELATOMS_GET_PAL_TYPE(UNKNOWN)) {
      sendPalOpenFailedMetric(palType);
    }
  };

  // Defer the metric sending callback to better ensure that the host can
  // receive this message, as this method may be called prior to chre::init()
  // completion.
  EventLoopManagerSingleton::get()->deferCallback(
      SystemCallbackType::DeferredMetricPostEvent, NestedDataPtr<PalType>(type),
      callback);
}

void TelemetryManager::collectSystemMetrics() {
  // TODO(b/207156504): Implement this

  scheduleMetricTimer();
}

void TelemetryManager::scheduleMetricTimer() {
  constexpr Seconds kDelay = Seconds(60 * 60 * 24);  // 24 hours
  auto callback = [](uint16_t /* eventType */, void * /* data */,
                     void * /* extraData */) {
    EventLoopManagerSingleton::get()
        ->getTelemetryManager()
        .collectSystemMetrics();
  };
  TimerHandle handle = EventLoopManagerSingleton::get()->setDelayedCallback(
      SystemCallbackType::DeferredMetricPostEvent, nullptr /* data */, callback,
      kDelay);
  if (handle == CHRE_TIMER_INVALID) {
    LOGE("Failed to set daily metric timer");
  }
}

}  // namespace chre
