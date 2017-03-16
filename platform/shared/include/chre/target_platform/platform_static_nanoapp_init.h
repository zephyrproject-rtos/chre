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

#ifndef CHRE_PLATFORM_SHARED_PLATFORM_STATIC_NANOAPP_INIT_H_
#define CHRE_PLATFORM_SHARED_PLATFORM_STATIC_NANOAPP_INIT_H_

/**
 * Initializes a nanoapp that is based on the shared implementation of
 * PlatformNanoappBase.
 *
 * @param appName the name of the nanoapp. This will be prefixed by gNanoapp
 * when creating the global instance of the nanoapp.
 */
#define PLATFORM_STATIC_NANOAPP_INIT(appName)          \
::chre::PlatformNanoapp gNanoapp##appName;             \
                                                       \
__attribute__((constructor))                           \
static void InitializeStaticNanoapp() {                \
  gNanoapp##appName.mStart = nanoappStart;             \
  gNanoapp##appName.mHandleEvent = nanoappHandleEvent; \
  gNanoapp##appName.mStop = nanoappStop;               \
}

#endif  // CHRE_PLATFORM_SHARED_PLATFORM_STATIC_NANOAPP_INIT_H_
