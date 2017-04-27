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

#ifndef CHRE_PLATFORM_SLPI_LOG_H_
#define CHRE_PLATFORM_SLPI_LOG_H_

#include "chre/platform/shared/platform_log.h"

#ifndef __FILENAME__
#define __FILENAME__ __FILE__
#endif

#ifndef FARF_MEDIUM
#define FARF_MEDIUM 1
#endif
#include "HAP_farf.h"

#define LOGE(fmt, ...) \
  FARF(ERROR, fmt, ##__VA_ARGS__); \
  chre::PlatformLogSingleton::get()->log("E " fmt, ##__VA_ARGS__)

#define LOGW(fmt, ...) \
  FARF(HIGH, fmt, ##__VA_ARGS__); \
  chre::PlatformLogSingleton::get()->log("W " fmt, ##__VA_ARGS__)

#define LOGI(fmt, ...) \
  FARF(MEDIUM, fmt, ##__VA_ARGS__); \
  chre::PlatformLogSingleton::get()->log("I " fmt, ##__VA_ARGS__)

#define LOGD(fmt, ...) \
  FARF(MEDIUM, fmt, ##__VA_ARGS__); \
  chre::PlatformLogSingleton::get()->log("D " fmt, ##__VA_ARGS__)

#endif  // CHRE_PLATFORM_SLPI_LOG_H_
