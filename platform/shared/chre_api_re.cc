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

#include "chre_api/chre/re.h"
#include "chre/core/event_loop.h"
#include "chre/platform/assert.h"
#include "chre/platform/context.h"
#include "chre/platform/system_time.h"

uint64_t chreGetTime() {
  return chre::SystemTime::getMonotonicTime().toRawNanoseconds();
}

uint64_t chreGetAppId(void) {
  chre::EventLoop *eventLoop = chre::getCurrentEventLoop();
  ASSERT(eventLoop);

  const chre::Nanoapp *currentApp = eventLoop->getCurrentNanoapp();
  ASSERT_LOG(currentApp, "%s called with no CHRE app context", __func__);
  return currentApp->getAppId();
}

uint32_t chreGetInstanceId(void) {
  chre::EventLoop *eventLoop = chre::getCurrentEventLoop();
  ASSERT(eventLoop);

  const chre::Nanoapp *currentApp = eventLoop->getCurrentNanoapp();
  ASSERT_LOG(currentApp, "%s called with no CHRE app context", __func__);
  return currentApp->getInstanceId();
}

uint32_t chreTimerSet(uint64_t duration, const void *cookie, bool oneShot) {
  chre::EventLoop *eventLoop = chre::getCurrentEventLoop();
  ASSERT(eventLoop);

  const chre::Nanoapp *currentApp = eventLoop->getCurrentNanoapp();
  ASSERT_LOG(currentApp, "%s called with no CHRE app context", __func__);
  return eventLoop->getTimerPool().setTimer(currentApp,
      chre::Nanoseconds(duration), cookie, oneShot);
}

bool chreTimerCancel(uint32_t timerId) {
  chre::EventLoop *eventLoop = chre::getCurrentEventLoop();
  ASSERT(eventLoop);

  const chre::Nanoapp *currentApp = eventLoop->getCurrentNanoapp();
  ASSERT_LOG(currentApp, "%s called with no CHRE app context", __func__);
  return eventLoop->getTimerPool().cancelTimer(currentApp, timerId);
}
