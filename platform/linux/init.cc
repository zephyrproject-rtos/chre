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
#include "chre/apps/sensor_world/sensor_world.h"
#include "chre/apps/timer_world/timer_world.h"
#include "chre/apps/wifi_world/wifi_world.h"
#include "chre/core/event.h"
#include "chre/core/event_loop.h"
#include "chre/core/event_loop_manager.h"
#include "chre/core/init.h"
#include "chre/core/nanoapp.h"
#include "chre/platform/context.h"
#include "chre/platform/log.h"
#include "chre/platform/system_timer.h"
#include "chre/platform/platform_nanoapp.h"
#include "chre/util/time.h"

#include <csignal>

using chre::Milliseconds;

namespace {

extern "C" void signalHandler(int sig) {
  (void) sig;
  LOGI("Stop request received");
  chre::getCurrentEventLoop()->stop();
}

}

int main() {
  chre::init();

  chre::PlatformNanoapp helloWorldPlatformNanoapp;
  helloWorldPlatformNanoapp.mStart = chre::app::helloWorldStart;
  helloWorldPlatformNanoapp.mHandleEvent = chre::app::helloWorldHandleEvent;
  helloWorldPlatformNanoapp.mStop = chre::app::helloWorldStop;

  chre::PlatformNanoapp sensorWorldPlatformNanoapp;
  sensorWorldPlatformNanoapp.mStart = chre::app::sensorWorldStart;
  sensorWorldPlatformNanoapp.mHandleEvent = chre::app::sensorWorldHandleEvent;
  sensorWorldPlatformNanoapp.mStop = chre::app::sensorWorldStop;

  chre::PlatformNanoapp timerWorldPlatformNanoapp;
  timerWorldPlatformNanoapp.mStart = chre::app::timerWorldStart;
  timerWorldPlatformNanoapp.mHandleEvent = chre::app::timerWorldHandleEvent;
  timerWorldPlatformNanoapp.mStop = chre::app::timerWorldStop;

  chre::PlatformNanoapp wifiWorldPlatformNanoapp;
  wifiWorldPlatformNanoapp.mStart = chre::app::wifiWorldStart;
  wifiWorldPlatformNanoapp.mHandleEvent = chre::app::wifiWorldHandleEvent;
  wifiWorldPlatformNanoapp.mStop = chre::app::wifiWorldStop;

  // Construct the event loop.
  chre::EventLoop& eventLoop = *chre::getCurrentEventLoop();

  // Start the hello world nanoapp.
  chre::Nanoapp helloWorldNanoapp(eventLoop.getNextInstanceId(),
      &helloWorldPlatformNanoapp);
  eventLoop.startNanoapp(&helloWorldNanoapp);
  helloWorldNanoapp.registerForBroadcastEvent(1);

  // Start the sensor world nanoapp.
  chre::Nanoapp sensorWorldNanoapp(eventLoop.getNextInstanceId(),
      &sensorWorldPlatformNanoapp);
  eventLoop.startNanoapp(&sensorWorldNanoapp);

  // Start the timer nanoapp.
  chre::Nanoapp timerWorldNanoapp(eventLoop.getNextInstanceId(),
      &timerWorldPlatformNanoapp);
  eventLoop.startNanoapp(&timerWorldNanoapp);

  // Start the wifi nanoapp.
  chre::Nanoapp wifiWorldNanoapp(eventLoop.getNextInstanceId(),
      &wifiWorldPlatformNanoapp);
  eventLoop.startNanoapp(&wifiWorldNanoapp);


  std::signal(SIGINT, signalHandler);
  eventLoop.run();

  eventLoop.stopNanoapp(&helloWorldNanoapp);
  eventLoop.stopNanoapp(&sensorWorldNanoapp);
  eventLoop.stopNanoapp(&timerWorldNanoapp);
  eventLoop.stopNanoapp(&wifiWorldNanoapp);

  chre::deinit();
  return 0;
}
