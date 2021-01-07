/*
 * Copyright (C) 2021 The Android Open Source Project
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

#ifndef CHRE_UTIL_SYSTEM_NAPP_PERMISSONS
#define CHRE_UTIL_SYSTEM_NAPP_PERMISSONS

namespace chre {

/**
 * Enum declaring the various nanoapp permissions that can be declared. Nanoapps
 * built against CHRE API v1.5+ must contain the respective permission for the
 * set of APIs they attempt to call. For example, CHRE_NANOAPP_USES_WIFI must
 * be declared by the nanoapp in order for it to make use of any WiFi APIs.
 *
 * The 10 most-significant bits are reserved for vendor use and must be used if
 * a vendor API allows access to privacy sensitive information that is guarded
 * by a permission on the Android side (e.g. location).
 */
enum class NanoappPermissions : uint32_t {
  NANOAPP_USES_AUDIO = 1,
  NANOAPP_USES_GNSS = 1 << 1,
  NANOAPP_USES_WIFI = 1 << 2,
  NANOAPP_USES_WWAN = 1 << 3,
};

}  // namespace chre

#endif  // CHRE_UTIL_SYSTEM_NAPP_PERMISSONS
