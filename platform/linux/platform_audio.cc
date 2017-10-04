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

#include "chre/platform/fatal_error.h"
#include "chre/platform/log.h"
#include "chre/util/dynamic_vector.h"

namespace chre {
namespace {

//! The list of audio sources provided by the simulator.
DynamicVector<UniquePtr<AudioSource>> gAudioSources;

}

void PlatformAudio::init() {}

void PlatformAudio::deinit() {}

bool PlatformAudio::getAudioSource(uint32_t handle,
                                   chreAudioSource *audioSource) {
  bool success = (handle < gAudioSources.size());
  if (success) {
    const auto& source = gAudioSources[handle];
    // TODO(P1-b9ff35): Ensure that name never exceeds 40 bytes in length.
    audioSource->name = source->audioFilename.c_str();
    audioSource->sampleRate =
        static_cast<uint32_t>(source->audioInfo.samplerate);
    audioSource->minBufferDuration = source->minBufferSize.toRawNanoseconds();
    audioSource->maxBufferDuration = source->maxBufferSize.toRawNanoseconds();
    audioSource->format = source->dataFormat;
  }

  return success;
}

void PlatformAudioBase::addAudioSource(UniquePtr<AudioSource>& source) {
  LOGI("Adding audio source - filename: %s, min buf size: %" PRIu64 "ms, "
       "max buf size: %" PRIu64 "ms", source->audioFilename.c_str(),
       Milliseconds(source->minBufferSize).getMilliseconds(),
       Milliseconds(source->maxBufferSize).getMilliseconds());
  auto& audioInfo = source->audioInfo;
  source->audioFile = sf_open(source->audioFilename.c_str(), SFM_READ,
                              &audioInfo);
  if (source->audioFile == nullptr) {
    FATAL_ERROR("Failed to open provided audio file %s",
                source->audioFilename.c_str());
  } else if ((audioInfo.format & SF_FORMAT_ULAW) == SF_FORMAT_ULAW) {
    source->dataFormat = CHRE_AUDIO_DATA_FORMAT_8_BIT_U_LAW;
  } else if ((audioInfo.format & SF_FORMAT_PCM_16) == SF_FORMAT_PCM_16) {
    source->dataFormat = CHRE_AUDIO_DATA_FORMAT_16_BIT_SIGNED_PCM;
  } else {
    FATAL_ERROR("Invalid format 0x%08x", audioInfo.format);
  }

  gAudioSources.push_back(std::move(source));
}

}  // namespace chre
