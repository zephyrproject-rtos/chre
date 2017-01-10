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

#include "chre/apps/hello_world/hello_world.h"
#include "chre/apps/timer_world/timer_world.h"
#include "chre/core/event.h"
#include "chre/core/event_loop.h"
#include "chre/core/event_loop_manager.h"
#include "chre/core/init.h"
#include "chre/core/nanoapp.h"
#include "chre/platform/log.h"
#include "chre/platform/system_timer.h"
#include "chre/platform/platform_nanoapp.h"
#include "chre/util/time.h"

#include <csignal>

using chre::Milliseconds;

namespace {

chre::EventLoop *gEventLoop = nullptr;

void delayedEventCallback(void *data) {
  LOGI("Got a delayed event callback");
  auto *eventLoop = static_cast<chre::EventLoop *>(data);
  eventLoop->postEvent(1, nullptr, nullptr);
}

void timerCallback(void *data) {
  LOGI("Got timer callback");
  auto *eventLoop = static_cast<chre::EventLoop *>(data);
  eventLoop->stop();
}

extern "C" void signalHandler(int sig) {
  (void) sig;
  LOGI("Stop request received");
  gEventLoop->stop();
}

}

namespace chre {

EventLoop *getCurrentEventLoop() {
  // note: on a multi-threaded implementation, we would likely use thread-local
  // storage here if available, or a map from thread ID --> eventLoop
  return gEventLoop;
}

}  // namespace chre

int main() {
  chre::init();

  chre::PlatformNanoapp helloWorldPlatformNanoapp;
  helloWorldPlatformNanoapp.mStart = chre::app::helloWorldStart;
  helloWorldPlatformNanoapp.mHandleEvent = chre::app::helloWorldHandleEvent;
  helloWorldPlatformNanoapp.mStop = chre::app::helloWorldStop;

  chre::PlatformNanoapp timerWorldPlatformNanoapp;
  timerWorldPlatformNanoapp.mStart = chre::app::timerWorldStart;
  timerWorldPlatformNanoapp.mHandleEvent = chre::app::timerWorldHandleEvent;
  timerWorldPlatformNanoapp.mStop = chre::app::timerWorldStop;

  // Construct the event loop.
  gEventLoop = chre::EventLoopManagerSingleton::get()->createEventLoop();
  chre::EventLoop& eventLoop = *gEventLoop;

  // Start the hello world nanoapp.
  chre::Nanoapp helloWorldNanoapp(eventLoop.getNextInstanceId(),
      &helloWorldPlatformNanoapp);
  eventLoop.startNanoapp(&helloWorldNanoapp);
  helloWorldNanoapp.registerForBroadcastEvent(1);

  // Start the timer nanoapp.
  chre::Nanoapp timerWorldNanoapp(eventLoop.getNextInstanceId(),
      &timerWorldPlatformNanoapp);
  eventLoop.startNanoapp(&timerWorldNanoapp);

  // Send an event to all nanoapps.
  eventLoop.postEvent(1, nullptr, nullptr);

  chre::SystemTimer delayedEventTimer;
  chre::SystemTimer sysTimer;
  if (!delayedEventTimer.init() || !sysTimer.init()) {
    LOGE("Couldn't init timer");
  } else if (
      !delayedEventTimer.set(delayedEventCallback, &eventLoop, Milliseconds(500))
          || !sysTimer.set(timerCallback, &eventLoop, Milliseconds(1000))) {
    LOGE("Couldn't set timer");
  } else {
    std::signal(SIGINT, signalHandler);
    eventLoop.run();
  }

  return 0;
}
