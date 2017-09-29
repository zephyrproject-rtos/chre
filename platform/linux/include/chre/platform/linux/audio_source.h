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

#ifndef CHRE_PLATFORM_LINUX_AUDIO_SOURCE_H_
#define CHRE_PLATFORM_LINUX_AUDIO_SOURCE_H_

#include <string>

#include "chre/util/non_copyable.h"
#include "chre/util/time.h"

namespace chre {

/**
 * Maintains the state of one audio source for the simulation environment.
 */
class AudioSource : public NonCopyable {
 public:
  /**
   * Constructs an audio source.
   *
   * @param audioFilename the filename for the file to open and playback.
   * @param minBufferSize the minimum buffer size, in seconds, to provide to a
   *        nanoapp.
   * @param maxBufferSize the maximum buffer size, in seconds, to provide to a
   *        nanoapp.
   */
  AudioSource(const std::string& audioFilename,
              double minBufferSize, double maxBufferSize);

  //! The audio file to open and playback to the nanoapp.
  const std::string audioFilename;

  //! The minimum buffer size for this audio source.
  const Nanoseconds minBufferSize;

  //! The maximum buffer size for this audio source.
  const Nanoseconds maxBufferSize;
};

}  // namespace chre

#endif  // CHRE_PLATFORM_LINUX_AUDIO_SOURCE_H_
