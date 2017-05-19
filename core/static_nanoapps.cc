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

#include "chre/apps/apps.h"
#include "chre/core/static_nanoapps.h"
#include "chre/util/macros.h"

namespace chre {

// The CHRE build variant can supply this macro to override the default list of
// static nanoapps. Most production variants will supply this macro as these
// nanoapps are mostly intended for testing and evaluation purposes.
#ifndef CHRE_VARIANT_SUPPLIES_STATIC_NANOAPP_LIST

//! The default list of static nanoapps to load.
const StaticNanoappInitFunction kStaticNanoappList[] = {
  initializeStaticNanoappGnssWorld,
  initializeStaticNanoappHelloWorld,
  initializeStaticNanoappImuCal,
  initializeStaticNanoappMessageWorld,
  initializeStaticNanoappSensorWorld,
  initializeStaticNanoappSpammer,
  initializeStaticNanoappTimerWorld,
  initializeStaticNanoappUnloadTester,
  initializeStaticNanoappWifiWorld,
  initializeStaticNanoappWwanWorld,
};

//! The size of the default static nanoapp list.
const size_t kStaticNanoappCount = ARRAY_SIZE(kStaticNanoappList);

#endif  // CHRE_VARIANT_SUPPLIES_STATIC_NANOAPP_LIST

void loadStaticNanoapps(EventLoop *eventLoop) {
  for (size_t i = 0; i < kStaticNanoappCount; i++) {
    UniquePtr<Nanoapp> nanoapp = kStaticNanoappList[i]();
    eventLoop->startNanoapp(nanoapp);
  }
}

}  // namespace chre
