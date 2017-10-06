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

#include "chre/util/nanoapp/audio.h"
#include "chre/util/nanoapp/log.h"

#define LOG_TAG "[AudioWorld]"

// TODO(P1-6f72cb): Exercise the CHRE Audio API once it is available.

#ifdef CHRE_NANOAPP_INTERNAL
namespace chre {
namespace {
#endif  // CHRE_NANOAPP_INTERNAL

bool nanoappStart() {
  LOGI("Started");

  struct chreAudioSource audioSource;
  for (uint32_t i = 0; chreAudioGetSource(i, &audioSource); i++) {
    LOGI("Found audio source '%s' with %" PRIu32 "Hz %s data - min buffer "
         "duration: %" PRIu64 "ns, max buffer duration: %" PRIu64 "ns",
         audioSource.name, audioSource.sampleRate,
         getChreAudioFormatString(audioSource.format),
         audioSource.minBufferDuration, audioSource.maxBufferDuration);
  }

  return true;
}

void nanoappHandleEvent(uint32_t senderInstanceId,
                        uint16_t eventType,
                        const void *eventData) {
  switch (eventType) {
    default:
      LOGW("Unknown event received");
      break;
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

CHRE_STATIC_NANOAPP_INIT(AudioWorld, chre::kAudioWorldAppId, 0);
#endif  // CHRE_NANOAPP_INTERNAL
