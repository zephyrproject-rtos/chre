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

#include <chre.h>
#include <cinttypes>

#include "chre/util/nanoapp/audio.h"
#include "chre/util/nanoapp/log.h"

#define LOG_TAG "[TimerWorld]"

#ifdef CHRE_NANOAPP_INTERNAL
namespace chre {
namespace {
#endif  // CHRE_NANOAPP_INTERNAL

uint32_t gOneShotTimerHandle;
uint32_t gCyclicTimerHandle;
int gCyclicTimerCount;

bool nanoappStart() {
  LOGI("App started on platform ID %" PRIx64, chreGetPlatformId());

  gOneShotTimerHandle = chreTimerSet(100000000 /* duration: 100ms */,
      &gOneShotTimerHandle /* data */,
      true /* oneShot */);
  gCyclicTimerHandle = chreTimerSet(150000000 /* duration: 150ms */,
      &gCyclicTimerHandle /* data */,
      false /* oneShot */);
  gCyclicTimerCount = 0;

  struct chreAudioSource audioSource;
  for (uint32_t i = 0; chreAudioGetSource(i, &audioSource); i++) {
    LOGI("Found audio source '%s' with %" PRIu32 "Hz %s data",
         audioSource.name, audioSource.sampleRate,
         chre::getChreAudioFormatString(audioSource.format));
    LOGI("  buffer duration: [%" PRIu64 "ns, %" PRIu64 "ns]",
        audioSource.minBufferDuration, audioSource.maxBufferDuration);

    if (i == 0) {
      // Only request audio data from the first source, but continue discovery.
      if (chreAudioConfigureSource(i, true,
          audioSource.minBufferDuration, audioSource.minBufferDuration)) {
        LOGI("Requested audio from handle %" PRIu32 " successfully", i);
      } else {
        LOGE("Failed to request audio from handle %" PRIu32, i);
      }
    }
  }

  return true;
}

void handleTimerEvent(const void *eventData) {
  const uint32_t *timerHandle = static_cast<const uint32_t *>(eventData);
  if (*timerHandle == gOneShotTimerHandle) {
    LOGI("One shot timer event received");
  } else if (*timerHandle == gCyclicTimerHandle) {
    LOGI("Cyclic timer event received");
    gCyclicTimerCount++;
    if (gCyclicTimerCount > 1) {
      chreTimerCancel(gCyclicTimerHandle);
    }
  }
}

void nanoappHandleEvent(uint32_t senderInstanceId,
                        uint16_t eventType,
                        const void *eventData) {
  switch (eventType) {
    case CHRE_EVENT_TIMER:
      handleTimerEvent(eventData);
      break;
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

CHRE_STATIC_NANOAPP_INIT(TimerWorld, chre::kTimerWorldAppId, 0);
#endif  // CHRE_NANOAPP_INTERNAL
