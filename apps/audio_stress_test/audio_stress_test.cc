/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include "chre/util/optional.h"
#include "chre/util/nanoapp/audio.h"
#include "chre/util/nanoapp/log.h"
#include "chre/util/time.h"

#define LOG_TAG "[AudioStress]"

/**
 * @file
 *
 * This nanoapp is designed to subscribe to audio for varying durations of
 * time and verify that audio data is delivered when it is expected to be. It is
 * designed to be loaded by a companion host binary that listens for messages
 * from this nanoapp to indicate failure or otherwise.
 */

using chre::Milliseconds;
using chre::Nanoseconds;
using chre::Optional;
using chre::Seconds;

namespace {

//! The required buffer size for the stress test.
constexpr Nanoseconds kBufferDuration = Nanoseconds(Seconds(2));

//! The required sample format for the stress test.
constexpr uint8_t kBufferFormat = CHRE_AUDIO_DATA_FORMAT_16_BIT_SIGNED_PCM;

//! The required sample rate for the stress test.
constexpr uint32_t kBufferSampleRate = 16000;

//! The list of durations to subscribe to audio for. Even durations are for when
//! audio is enabled and odd is for when audio is disabled.
constexpr Milliseconds kStressPlan[] = {
  // Enabled, Disabled
  Milliseconds(1000), Milliseconds(2000),
  Milliseconds(10000), Disabled(1000),
};

//! The discovered audio handle found at startup.
uint32_t gAudioHandle;

//! The current position in the stress plan.
size_t gTestPosition = 0;

//! The timer handle to advance through the stress test.
uint32_t gTimerHandle;

/**
 * Discovers an audio source to use for the stress test. The gAudioHandle will
 * be set if the audio source was found.
 *
 * @return true if a matching source was discovered successfully.
 */
bool discoverAudioHandle() {
  bool success = false;
  struct chreAudioSource source;
  for (uint32_t i = 0; !success && chreAudioGetSource(i, &source); i++) {
    LOGI("Found audio source '%s' with %" PRIu32 "Hz %s data",
         source.name, source.sampleRate,
         chre::getChreAudioFormatString(source.format));
    LOGI("  buffer duration: [%" PRIu64 "ns, %" PRIu64 "ns]",
        source.minBufferDuration, source.maxBufferDuration);

    if (source.sampleRate == kBufferSampleRate
        && source.minBufferDuration <= kBufferDuration.toRawNanoseconds()
        && source.maxBufferDuration >= kBufferDuration.toRawNanoseconds()
        && source.format == kBufferFormat) {
      gAudioHandle = i;
      success = true;
    }
  }

  if (!success) {
    LOGW("Failed to find suitable audio source");
  }

  return success;
}

bool advanceTestPosition() {
  gTimerHandle = chreTimerSet(kStressPlan[gTestPosition++].toRawNanoseconds(),
                              nullptr, true /* oneShot */);
  bool success = (gTimerHandle != CHRE_TIMER_INVALID);
  if (!success) {
    LOGE("Failed to set test timer");
  }

  return success;
}

bool startStressTest() {
  return true;
}

}  // namespace


bool nanoappStart() {
  LOGI("start");
  return (discoverAudioHandle() && startStressTest());
}

void nanoappHandleEvent(uint32_t senderInstanceId,
                        uint16_t eventType,
                        const void *eventData) {

}

void nanoappEnd() {
  LOGI("stop");
}
