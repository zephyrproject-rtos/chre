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

#include "chre/core/gnss_request_manager.h"

#include "chre/platform/assert.h"

namespace chre {

uint32_t GnssRequestManager::getCapabilities() {
  return mPlatformGnss.getCapabilities();
}

bool GnssRequestManager::startLocationSession(Nanoapp *nanoapp,
                                              Milliseconds minInterval,
                                              Milliseconds minTimeToNextFix,
                                              const void *cookie) {
  CHRE_ASSERT(nanoapp);
  return configureLocationSession(nanoapp, true /* enable */, minInterval,
                                  minTimeToNextFix, cookie);
}

bool GnssRequestManager::stopLocationSession(Nanoapp *nanoapp,
                                             const void *cookie) {
  CHRE_ASSERT(nanoapp);
  return configureLocationSession(nanoapp, false /* enable */, Milliseconds(),
                                  Milliseconds(), cookie);
}

bool GnssRequestManager::configureLocationSession(
    Nanoapp *nanoapp, bool enable, Milliseconds minInterval,
    Milliseconds minTimeToFirstFix, const void *cookie) {
  // TODO: Implement this.
  return false;
}

}  // namespace chre
