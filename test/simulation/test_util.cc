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

#include "chre/core/event_loop_manager.h"
#include "chre/util/macros.h"
#include "chre/util/memory.h"
#include "chre_api/chre/version.h"

namespace chre {

UniquePtr<Nanoapp> createStaticNanoapp(
    const char *name, uint64_t appId, uint32_t appVersion, uint32_t appPerms,
    decltype(nanoappStart) *startFunc,
    decltype(nanoappHandleEvent) *handleEventFunc,
    decltype(nanoappEnd) *endFunc) {
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
  appInfo.entryPoints.start = startFunc;
  appInfo.entryPoints.handleEvent = handleEventFunc;
  appInfo.entryPoints.end = endFunc;
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

void loadNanoapp(const char *name, uint64_t appId, uint32_t appVersion,
                 uint32_t appPerms, decltype(nanoappStart) *startFunc,
                 decltype(nanoappHandleEvent) *handleEventFunc,
                 decltype(nanoappEnd) *endFunc) {
  UniquePtr<Nanoapp> nanoapp = createStaticNanoapp(
      name, appId, appVersion, appPerms, startFunc, handleEventFunc, endFunc);
  EventLoopManagerSingleton::get()->deferCallback(
      SystemCallbackType::FinishLoadingNanoapp, std::move(nanoapp),
      testFinishLoadingNanoappCallback);
}

void unloadNanoapp(uint64_t appId) {
  uint64_t *ptr = memoryAlloc<uint64_t>();
  ASSERT_NE(ptr, nullptr);
  *ptr = appId;
  EventLoopManagerSingleton::get()->deferCallback(
      SystemCallbackType::HandleUnloadNanoapp, ptr,
      testFinishUnloadingNanoappCallback);
}

void testFinishLoadingNanoappCallback(SystemCallbackType /* type */,
                                      UniquePtr<Nanoapp> &&nanoapp) {
  EventLoopManagerSingleton::get()->getEventLoop().startNanoapp(nanoapp);
}

void testFinishUnloadingNanoappCallback(uint16_t /* type */, void *data,
                                        void * /* extraData */) {
  EventLoop &eventLoop = EventLoopManagerSingleton::get()->getEventLoop();
  uint16_t instanceId = 0;
  uint64_t *appId = static_cast<uint64_t *>(data);
  eventLoop.findNanoappInstanceIdByAppId(*appId, &instanceId);
  eventLoop.unloadNanoapp(instanceId, true);
  memoryFree(data);
}

}  // namespace chre
