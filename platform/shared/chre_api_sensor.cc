/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include "chre/core/event_loop_manager.h"
#include "chre/core/sensor_request.h"
#include "chre/util/time.h"
#include "chre/util/macros.h"
#include "chre_api/chre/sensor.h"

using chre::EventLoopManager;
using chre::EventLoopManagerSingleton;
using chre::Nanoseconds;
using chre::SensorMode;
using chre::SensorRequest;
using chre::SensorType;

using chre::getSensorModeFromEnum;
using chre::getSensorTypeFromUnsignedInt;

#if defined(CHRE_SLPI_SEE) && defined(CHRE_SLPI_UIMG_ENABLED)
namespace {
constexpr uint8_t kBigImageAccelSensorType =
    (CHRE_SENSOR_TYPE_VENDOR_START + 3);
} //  anonymous namespace
#endif  // defined(CHRE_SLPI_SEE) && defined(CHRE_SLPI_UIMG_ENABLED)

DLL_EXPORT bool chreSensorFindDefault(uint8_t sensorType, uint32_t *handle) {
#if defined(CHRE_SLPI_SEE) && defined(CHRE_SLPI_UIMG_ENABLED)
  // HACK: as SEE does not support software batching in uimg via QCM/uQSockets,
  // reroute requests for accelerometer from a big image nanoapp to a separate
  // sensor type internally. Accel is the only always-on sensor used today by
  // big image nanoapps, and this change allows these requests to transparently
  // go to a separate sensor implementation that supports uimg batching via
  // CM/QMI.
  // TODO(P2-5673a9): work with QC to determine a better long-term solution
  chre::Nanoapp *nanoapp = EventLoopManager::validateChreApiCall(__func__);
  if (!nanoapp->isUimgApp()) {
    if (sensorType == CHRE_SENSOR_TYPE_ACCELEROMETER) {
      sensorType = kBigImageAccelSensorType;
    } else if (sensorType == kBigImageAccelSensorType) {
      // Since we have an accompanying hack in PlatformNanoapp::handleEvent(),
      // hide the vendor sensor type from big image nanoapps as we're unable to
      // deliver events for it
      return false;
    }
  }
#endif  // defined(CHRE_SLPI_SEE) && defined(CHRE_SLPI_UIMG_ENABLED)

  SensorType validatedSensorType = getSensorTypeFromUnsignedInt(sensorType);
  return (validatedSensorType != SensorType::Unknown
      && EventLoopManagerSingleton::get()->getSensorRequestManager()
          .getSensorHandle(validatedSensorType, handle));
}

DLL_EXPORT bool chreGetSensorInfo(uint32_t sensorHandle,
                                  struct chreSensorInfo *info) {
  CHRE_ASSERT(info);

  chre::Nanoapp *nanoapp = EventLoopManager::validateChreApiCall(__func__);

  bool success = false;
  if (info != nullptr) {
    success = EventLoopManagerSingleton::get()->getSensorRequestManager().
        getSensorInfo(sensorHandle, *nanoapp, info);

    // The distinction between big/uimg accel should be abstracted away from
    // big image nanoapps, so overwrite any platform implementation here.
#if defined(CHRE_SLPI_SEE) && defined(CHRE_SLPI_UIMG_ENABLED)
    if (!nanoapp->isUimgApp() && info->sensorType == kBigImageAccelSensorType) {
      info->sensorType = CHRE_SENSOR_TYPE_ACCELEROMETER;
    }
#endif  // defined(CHRE_SLPI_SEE) && defined(CHRE_SLPI_UIMG_ENABLED)

  }
  return success;
}

DLL_EXPORT bool chreGetSensorSamplingStatus(
    uint32_t sensorHandle, struct chreSensorSamplingStatus *status) {
  CHRE_ASSERT(status);

  bool success = false;
  if (status != nullptr) {
    success = EventLoopManagerSingleton::get()->getSensorRequestManager().
        getSensorSamplingStatus(sensorHandle, status);
  }
  return success;
}

DLL_EXPORT bool chreSensorConfigure(uint32_t sensorHandle,
                                    enum chreSensorConfigureMode mode,
                                    uint64_t interval, uint64_t latency) {
  chre::Nanoapp *nanoapp = EventLoopManager::validateChreApiCall(__func__);
  SensorMode sensorMode = getSensorModeFromEnum(mode);
  SensorRequest sensorRequest(nanoapp, sensorMode, Nanoseconds(interval),
                              Nanoseconds(latency));
  return EventLoopManagerSingleton::get()->getSensorRequestManager()
      .setSensorRequest(nanoapp, sensorHandle, sensorRequest);
}
