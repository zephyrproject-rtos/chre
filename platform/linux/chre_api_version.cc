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

#include "chre_api/chre/version.h"

#include "chre/util/id_from_string.h"

//! The vendor ID of the Linux variant: "Googl".
constexpr uint64_t kVendorIdGoogle = createIdFromString("Googl");

//! The vendor platform ID of the Linux variant is one. All other
//  Google implementations will be greater than one. Zero is reserved.
constexpr uint64_t kGoogleLinuxPlatformId = 0x001;

//! The patch version of the Linux platform.
constexpr uint32_t kLinuxPatchVersion = 0x0001;

uint32_t chreGetApiVersion(void) {
  return CHRE_API_VERSION_1_1;
}

uint32_t chreGetVersion(void) {
  return chreGetApiVersion() | kLinuxPatchVersion;
}

uint64_t chreGetPlatformId(void) {
  return kVendorIdGoogle | kGoogleLinuxPlatformId;
}
