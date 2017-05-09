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

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "chre_api/chre/event.h"
#include "chre_api/chre/re.h"
#include "chre/core/event_loop_manager.h"
#include "chre/core/host_comms_manager.h"
#include "chre/platform/context.h"
#include "chre/platform/fatal_error.h"
#include "chre/platform/log.h"

using chre::EventLoop;
using chre::EventLoopManager;
using chre::EventLoopManagerSingleton;
using chre::Nanoapp;

void chreAbort(uint32_t abortCode) {
  Nanoapp *nanoapp = EventLoopManager::validateChreApiCall(__func__);

  // TODO: we should cleanly unload the nanoapp, release all of its resources,
  // and send an abort notification to the host so as to localize the impact to
  // the calling nanoapp
  if (nanoapp == nullptr) {
    FATAL_ERROR("chreAbort called in unknown context");
  } else {
    FATAL_ERROR("chreAbort called by app 0x%016" PRIx64, nanoapp->getAppId());
  }
}

bool chreSendEvent(uint16_t eventType, void *eventData,
                   chreEventCompleteFunction *freeCallback,
                   uint32_t targetInstanceId) {
  EventLoop *eventLoop;
  Nanoapp *nanoapp = EventLoopManager::validateChreApiCall(__func__,
                                                           &eventLoop);

  // Prevent an app that is in the process of being unloaded from generating new
  // events
  bool success = false;
  if (eventLoop->currentNanoappIsStopping()) {
    LOGW("Rejecting event from app instance %" PRIu32 " because it's stopping",
         nanoapp->getInstanceId());
  } else {
    success = EventLoopManagerSingleton::get()->postEvent(
        eventType, eventData, freeCallback, nanoapp->getInstanceId(),
        targetInstanceId);
  }

  if (!success && freeCallback != nullptr) {
    freeCallback(eventType, eventData);
  }
  return success;
}

bool chreSendMessageToHost(void *message, uint32_t messageSize,
                           uint32_t messageType,
                           chreMessageFreeFunction *freeCallback) {
  return chreSendMessageToHostEndpoint(
      message, static_cast<size_t>(messageSize), messageType,
      CHRE_HOST_ENDPOINT_BROADCAST, freeCallback);
}

bool chreSendMessageToHostEndpoint(void *message, size_t messageSize,
                                   uint32_t messageType, uint16_t hostEndpoint,
                                   chreMessageFreeFunction *freeCallback) {
  EventLoop *eventLoop;
  Nanoapp *nanoapp = EventLoopManager::validateChreApiCall(__func__,
                                                           &eventLoop);

  bool success = false;
  if (eventLoop->currentNanoappIsStopping()) {
    LOGW("Rejecting message to host from app instance %" PRIu32 " because it's "
         "stopping", nanoapp->getInstanceId());
  } else {
    auto& hostCommsManager =
        EventLoopManagerSingleton::get()->getHostCommsManager();
    success = hostCommsManager.sendMessageToHostFromNanoapp(
        nanoapp, message, messageSize, messageType, hostEndpoint, freeCallback);
  }

  if (!success && freeCallback != nullptr) {
    freeCallback(message, messageSize);
  }

  return success;
}

bool chreGetNanoappInfoByAppId(uint64_t appId, struct chreNanoappInfo *info) {
  EventLoopManager *eventLoopManager = EventLoopManagerSingleton::get();
  return eventLoopManager->populateNanoappInfoForAppId(appId, info);
}

bool chreGetNanoappInfoByInstanceId(uint32_t instanceId,
                                    struct chreNanoappInfo *info) {
  EventLoopManager *eventLoopManager = EventLoopManagerSingleton::get();
  return eventLoopManager->populateNanoappInfoForInstanceId(instanceId, info);
}

void chreConfigureNanoappInfoEvents(bool enable) {
  chre::Nanoapp *nanoapp = EventLoopManager::validateChreApiCall(__func__);
  nanoapp->configureNanoappInfoEvents(enable);
}

void chreLog(enum chreLogLevel level, const char *formatStr, ...) {
  char logBuf[512];
  va_list args;

  va_start(args, formatStr);
  vsnprintf(logBuf, sizeof(logBuf), formatStr, args);
  va_end(args);

  switch (level) {
    case CHRE_LOG_ERROR:
      LOGE("%s", logBuf);
      break;
    case CHRE_LOG_WARN:
      LOGW("%s", logBuf);
      break;
    case CHRE_LOG_INFO:
      LOGI("%s", logBuf);
      break;
    case CHRE_LOG_DEBUG:
    default:
      LOGD("%s", logBuf);
  }
}
