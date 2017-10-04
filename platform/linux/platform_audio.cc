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

#include <cinttypes>

#include "chre/platform/log.h"
#include "chre/util/dynamic_vector.h"

namespace chre {
namespace {

//! The list of audio sources provided by the simulator.
DynamicVector<UniquePtr<AudioSource>> gAudioSources;

}

void PlatformAudio::init() {
  // TODO: Init libsndfile.
}

void PlatformAudio::deinit() {
  // TODO: Deinit libsndfile.
}

bool PlatformAudio::getAudioSource(uint32_t handle,
                                   chreAudioSource *audioSource) {
  bool success = (handle < gAudioSources.size());
  if (success) {
    // TODO(P1-ca44e0): Read sample rate and format from the WAV file.
    const auto& source = gAudioSources[handle];
    audioSource->name = source->audioFilename.c_str();
    audioSource->minBufferDuration = source->minBufferSize.toRawNanoseconds();
    audioSource->maxBufferDuration = source->maxBufferSize.toRawNanoseconds();
  }

  return success;
}

void PlatformAudioBase::addAudioSource(UniquePtr<AudioSource>& source) {
  LOGI("Adding audio source - filename: %s, min buf size: %" PRIu64 "ms, "
       "max buf size: %" PRIu64 "ms", source->audioFilename.c_str(),
       Milliseconds(source->minBufferSize).getMilliseconds(),
       Milliseconds(source->maxBufferSize).getMilliseconds());
  gAudioSources.push_back(std::move(source));
}

}  // namespace chre
