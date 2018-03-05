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

#include "chre/platform/platform_audio.h"

#include <cstring>

#include "chre/platform/log.h"
#include "wcd_spi.h"

static_assert(
    sizeof(wcd_spi_audio_source_s) == sizeof(struct chreAudioSource),
    "WCD SPI/CHRE audio sources must be equal in size");
static_assert(
    offsetof(wcd_spi_audio_source_s, name)
        == offsetof(struct chreAudioSource, name),
    "WCD SPI/CHRE audio source name must have the same offset");
static_assert(
    offsetof(wcd_spi_audio_source_s, sample_rate_hz)
        == offsetof(struct chreAudioSource, sampleRate),
    "WCD SPI/CHRE audio source sample rate must have the same offset");
static_assert(
    offsetof(wcd_spi_audio_source_s, min_buffer_duration_ns)
        == offsetof(struct chreAudioSource, minBufferDuration),
    "WCD SPI/CHRE audio source min buffer duration must have the same offset");
static_assert(
    offsetof(wcd_spi_audio_source_s, max_buffer_duration_ns)
        == offsetof(struct chreAudioSource, maxBufferDuration),
    "WCD SPI/CHRE audio source max buffer duration must have the same offset");
static_assert(
    offsetof(wcd_spi_audio_source_s, format)
        == offsetof(struct chreAudioSource, format),
    "WCD SPI/CHRE audio source format must have the same offset");

namespace chre {

void handleWcdSpiAudioDataEvent(const wcd_spi_audio_data_event_s *event) {
  LOGD("WCD SPI audio data callback");
}

PlatformAudio::PlatformAudio() {
  wcd_spi_client_init(handleWcdSpiAudioDataEvent);
}

PlatformAudio::~PlatformAudio() {
  wcd_spi_client_deinit();
}

bool PlatformAudio::requestAudioDataEvent(uint32_t handle,
                                          uint32_t numSamples,
                                          Nanoseconds eventDelay) {
  // TODO(P1-f3f9a0): Implement this.
  return false;
}

void PlatformAudio::cancelAudioDataEventRequest(uint32_t handle) {
  // TODO(P1-f3f9a0): Implement this.
}

void PlatformAudio::releaseAudioDataEvent(struct chreAudioDataEvent *event) {
  // TODO(P1-f3f9a0): Implement this.
}

size_t PlatformAudio::getSourceCount() {
  return wcd_spi_client_get_source_count();
}

bool PlatformAudio::getAudioSource(uint32_t handle,
                                   chreAudioSource *source) {
  wcd_spi_audio_source_s wcd_spi_audio_source;
  bool result = wcd_spi_client_get_source(handle, &wcd_spi_audio_source);
  if (result) {
    // The WCD SPI and CHRE source definitions are binary compatible so a simple
    // memcpy will suffice.
    memcpy(source, &wcd_spi_audio_source, sizeof(*source));
  }

  return result;
}

}  // namespace chre
