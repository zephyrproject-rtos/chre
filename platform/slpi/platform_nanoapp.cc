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

#include "chre/platform/platform_nanoapp.h"

namespace chre {

PlatformNanoapp::~PlatformNanoapp() {}

bool PlatformNanoapp::start() {
  return mAppInfo->entryPoints.start();
}

void PlatformNanoapp::handleEvent(uint32_t senderInstanceId,
                                  uint16_t eventType,
                                  const void *eventData) {
  mAppInfo->entryPoints.handleEvent(senderInstanceId, eventType, eventData);
}

void PlatformNanoapp::end() {
  mAppInfo->entryPoints.end();
}

void PlatformNanoappBase::loadStatic(const struct chreNslNanoappInfo *appInfo) {
  mAppInfo = appInfo;
}

uint64_t PlatformNanoapp::getAppId() const {
  return mAppInfo->appId;
}

uint32_t PlatformNanoapp::getAppVersion() const {
  return mAppInfo->appVersion;
}

uint32_t PlatformNanoapp::getTargetApiVersion() const {
  return mAppInfo->targetApiVersion;
}

bool PlatformNanoapp::isSystemNanoapp() const {
  return mAppInfo->isSystemNanoapp;
}

}  // namespace chre
