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

#include "chre_api/chre/audio.h"

#include "chre/core/event_loop_manager.h"
#include "chre/util/macros.h"

using chre::EventLoopManagerSingleton;

DLL_EXPORT bool chreAudioGetSource(uint32_t handle,
                                   struct chreAudioSource *audioSource) {
  return EventLoopManagerSingleton::get()->getAudioRequestManager()
      .getAudioSource(handle, audioSource);
}
