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

#include "test_util.h"

#include <gtest/gtest.h>

#include "chre/util/macros.h"
#include "chre_api/chre/version.h"

namespace chre {

UniquePtr<Nanoapp> createStaticNanoapp(
    const char *name, uint64_t appId, uint32_t appVersion, uint32_t appPerms,
    chreNanoappStartFunction *nanoappStart,
    chreNanoappHandleEventFunction *nanoappHandleEvent,
    chreNanoappEndFunction *nanoappEnd) {
  UniquePtr<Nanoapp> nanoapp = MakeUnique<Nanoapp>();
  static struct chreNslNanoappInfo appInfo;
  appInfo.magic = CHRE_NSL_NANOAPP_INFO_MAGIC;
  appInfo.structMinorVersion = CHRE_NSL_NANOAPP_INFO_STRUCT_MINOR_VERSION;
  appInfo.targetApiVersion = CHRE_API_VERSION;
  appInfo.vendor = "Google";
  appInfo.name = name;
  appInfo.isSystemNanoapp = true;
  appInfo.isTcmNanoapp = true;
  appInfo.appId = appId;
  appInfo.appVersion = appVersion;
  appInfo.entryPoints.start = nanoappStart;
  appInfo.entryPoints.handleEvent = nanoappHandleEvent;
  appInfo.entryPoints.end = nanoappEnd;
  appInfo.appVersionString = "<undefined>";
  appInfo.appPermissions = appPerms;
  EXPECT_FALSE(nanoapp.isNull());
  nanoapp->loadStatic(&appInfo);

  return nanoapp;
}

bool defaultNanoappStart() {
  return true;
}

void defaultNanoappHandleEvent(uint32_t senderInstanceId, uint16_t eventType,
                               const void *eventData) {
  UNUSED_VAR(senderInstanceId);
  UNUSED_VAR(eventType);
  UNUSED_VAR(eventData);
}

void defaultNanoappEnd(){};

}  // namespace chre
