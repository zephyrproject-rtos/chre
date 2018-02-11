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

#include "chre/platform/shared/nanoapp_support_lib_dso.h"

#include <chre.h>

#include "chre/util/macros.h"

/**
 * @file
 * The Nanoapp Support Library (NSL) that gets built with nanoapps to act as an
 * intermediary to the reference CHRE implementation. It provides hooks so the
 * app can be registered with the system, and also provides a layer where we can
 * implement cross-version compatibility features as needed.
 */

#ifdef CHRE_SLPI_UIMG_ENABLED
static const int kIsTcmNanoapp = 1;
#else
static const int kIsTcmNanoapp = 0;
#endif  // CHRE_SLPI_UIMG_ENABLED

DLL_EXPORT const struct chreNslNanoappInfo _chreNslDsoNanoappInfo = {
  .magic = CHRE_NSL_NANOAPP_INFO_MAGIC,
  .structMinorVersion = CHRE_NSL_NANOAPP_INFO_STRUCT_MINOR_VERSION,
  .targetApiVersion = CHRE_API_VERSION,

  // These values are supplied by the build environment
  .vendor = NANOAPP_VENDOR_STRING,
  .name = NANOAPP_NAME_STRING,
  .isSystemNanoapp = NANOAPP_IS_SYSTEM_NANOAPP,
  .isTcmNanoapp = kIsTcmNanoapp,
  .appId = NANOAPP_ID,
  .appVersion = NANOAPP_VERSION,

  .entryPoints = {
    .start = nanoappStart,
    .handleEvent = nanoappHandleEvent,
    .end = nanoappEnd,
  },
};

// New symbols introduced in CHRE API v1.2; default implementations included for
// backwards compatibility. Note that we don't presently include symbols for
// v1.1 as the current set of nanoapps using this NSL do not target v1.0
// implementations.

WEAK_SYMBOL bool chreAudioGetSource(uint32_t handle,
                                    struct chreAudioSource *audioSource) {
  return false;
}

WEAK_SYMBOL bool chreAudioConfigureSource(uint32_t handle, bool enable,
                                          uint64_t bufferDuration,
                                          uint64_t deliveryInterval) {
  return false;
}

WEAK_SYMBOL bool chreAudioGetStatus(uint32_t handle,
                                    struct chreAudioSourceStatus *status) {
  return false;
}

WEAK_SYMBOL void chreConfigureHostSleepStateEvents(bool enable) {}

WEAK_SYMBOL bool chreIsHostAwake(void) {
  return false;
}

WEAK_SYMBOL bool chreGnssConfigureLocationMonitor(bool enable) {
  return false;
}

WEAK_SYMBOL bool chreWifiRequestRangingAsync(
    const struct chreWifiRangingParams *params, const void *cookie) {
  return false;
}

