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

#ifndef CHRE_CORE_AUDIO_REQUEST_MANAGER_H_
#define CHRE_CORE_AUDIO_REQUEST_MANAGER_H_


#include <cstdint>

#include "chre_api/chre/audio.h"
#include "chre/platform/platform_audio.h"
#include "chre/util/dynamic_vector.h"
#include "chre/util/non_copyable.h"

namespace chre {

/**
 * Manages requests for audio resources from nanoapps and multiplexes these
 * requests into the platform-specific implementation of the audio subsystem.
 */
class AudioRequestManager : public NonCopyable {
 public:
  /**
   * Sets up the initial condition for the audio request manager including
   * initial allocation of requests managers.
   */
  AudioRequestManager();

 private:
  /**
   * Tracks requests for an audio source from individual nanoapps. The handle of
   * the audio source is implied by the array index of this object in a
   * container type.
   */
  struct AudioRequests {
    // TODO(P1-678d91): Add relevant members to track requests from nanoapps.
  };

  //! Maps audio handles to requests from multiple nanoapps for an audio source.
  //! The array index implies the audio handle which is being managed.
  DynamicVector<AudioRequests> mAudioRequests;

  //! The instance of platform audio that is managed by this AudioRequestManager
  //! and used to service requests for audio data.
  PlatformAudio mPlatformAudio;
};

}  // namespace chre

#endif  // CHRE_CORE_AUDIO_REQUEST_MANAGER_H_
