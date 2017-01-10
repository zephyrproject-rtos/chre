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

#include "chre/platform/sensor_context.h"

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

}  // anonymous namespace

namespace chre {

EventLoop *getCurrentEventLoop() {
  // note: on a multi-threaded implementation, we would likely use thread-local
  // storage here if available, or a map from thread ID --> taskrunner
  return gEventLoop;
}

}  // namespace chre

/**
 * The main entry point to the SLPI CHRE runtime.
 *
 * This method is invoked automatically via FastRPC and must be externed for
 * C-linkage.
 */
extern "C" int chre_init() {
  chre::init();

  chre::DynamicVector<chre::PlatformSensor> sensors;
  if (!chre::SensorContext::getSensors(&sensors)) {
    LOGE("Failed to obtain the list of platform sensors\n");
  }

  for (size_t i = 0; i < sensors.size(); i++) {
    LOGD("Found sensor %d (%s)", sensors[i].getSensorType(),
         getSensorTypeName(sensors[i].getSensorType()));
  }

  chre::PlatformNanoapp helloWorldPlatformNanoapp;
  helloWorldPlatformNanoapp.mStart = chre::app::helloWorldStart;
  helloWorldPlatformNanoapp.mHandleEvent = chre::app::helloWorldHandleEvent;
  helloWorldPlatformNanoapp.mStop = chre::app::helloWorldStop;

  chre::PlatformNanoapp timerWorldPlatformNanoapp;
  timerWorldPlatformNanoapp.mStart = chre::app::timerWorldStart;
  timerWorldPlatformNanoapp.mHandleEvent = chre::app::timerWorldHandleEvent;
  timerWorldPlatformNanoapp.mStop = chre::app::timerWorldStop;

  gEventLoop = chre::EventLoopManagerSingleton::get()->createEventLoop();

  // Start the hello world nanoapp.
  chre::Nanoapp helloWorldNanoapp(gEventLoop->getNextInstanceId(),
      &helloWorldPlatformNanoapp);
  helloWorldNanoapp.registerForBroadcastEvent(1);
  gEventLoop->startNanoapp(&helloWorldNanoapp);

  // Start the timer nanoapp.
  chre::Nanoapp timerWorldNanoapp(gEventLoop->getNextInstanceId(),
      &timerWorldPlatformNanoapp);
  gEventLoop->startNanoapp(&timerWorldNanoapp);

  // Send an event to all nanoapps.
  gEventLoop->postEvent(1, nullptr, nullptr);

  chre::SystemTimer delayedEventTimer;
  chre::SystemTimer sysTimer;
  if (!delayedEventTimer.init() || !sysTimer.init()) {
    LOGE("Couldn't init timer");
  } else if (
      !delayedEventTimer.set(delayedEventCallback, gEventLoop, Milliseconds(500))
          || !sysTimer.set(timerCallback, gEventLoop, Milliseconds(1000))) {
    LOGE("Couldn't set timer");
  } else {
    gEventLoop->run();
  }

  return 0;
}
