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

#ifndef CHRE_PLATFORM_SLPI_PLATFORM_NANOAPP_BASE_H_
#define CHRE_PLATFORM_SLPI_PLATFORM_NANOAPP_BASE_H_

#include <cstdint>

#include "chre/platform/shared/nanoapp_support_lib_dso.h"
#include "chre/util/entry_points.h"

namespace chre {

/**
 * SLPI-specific nanoapp functionality.
 */
class PlatformNanoappBase {
 public:
  /**
   * Associate this PlatformNanoapp with a nanoapp that is statically built into
   * the CHRE binary with the given app info structure.
   */
  void loadStatic(const struct chreNslNanoappInfo *appInfo);

 protected:
  //! Pointer to the app info structure within this nanoapp
  const struct chreNslNanoappInfo *mAppInfo = nullptr;
};

}  // namespace chre

#endif  // CHRE_PLATFORM_SLPI_PLATFORM_NANOAPP_BASE_H_
