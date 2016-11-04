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
#include "chre/core/event.h"
#include "chre/core/event_loop.h"
#include "chre/core/init.h"
#include "chre/core/memory_pool.h"
#include "chre/core/nanoapp.h"
#include "chre/platform/log.h"
#include "chre/platform/system_timer.h"
#include "chre/platform/platform_nanoapp.h"
#include "chre/util/time.h"

#include <csignal>

using chre::Milliseconds;

namespace {

chre::EventLoop *gTaskRunner = nullptr;

void delayedEventTimerCallback(void *data) {
  LOGI("Got a delayed event callback");
  chre::Event *event = new(chre::MemoryPool<chre::Event>::allocate())
      chre::Event(1, nullptr, nullptr);
  auto *runner = static_cast<chre::EventLoop *>(data);
  runner->postEvent(event);
}

void timerCallback(void *data) {
  LOGI("Got timer callback");
  auto *runner = static_cast<chre::EventLoop *>(data);
  runner->stop();
}

extern "C" void signalHandler(int sig) {
  (void) sig;
  LOGI("Stop request received");
  gTaskRunner->stop();
}

}

namespace chre {

EventLoop *currentEventLoop() {
  // note: on a multi-threaded implementation, we would likely use thread-local
  // storage here if available, or a map from thread ID --> taskrunner
  return gTaskRunner;
}

}

int main() {
  chre::init();

  chre::PlatformNanoapp platformNanoapp;
  platformNanoapp.mStart = chre::app::helloWorldStart;
  platformNanoapp.mHandleEvent = chre::app::helloWorldHandleEvent;
  platformNanoapp.mStop = chre::app::helloWorldStop;

  // TODO: Move/refactor most of this to shared init since it is portable.
  chre::EventLoop runner;
  gTaskRunner = &runner;
  chre::Nanoapp nanoapp(runner.getNextInstanceId(), &platformNanoapp);

  runner.startNanoapp(&nanoapp);

  chre::Event *event = new(chre::MemoryPool<chre::Event>::allocate())
      chre::Event(1, nullptr, nullptr);
  nanoapp.registerForBroadcastEvent(1);
  runner.postEvent(event);

  chre::SystemTimer delayedEventTimer(delayedEventTimerCallback, &runner);
  chre::SystemTimer sysTimer(timerCallback, &runner);
  if (!delayedEventTimer.init() || !sysTimer.init()) {
    LOGE("Couldn't init timer");
  } else if (!delayedEventTimer.set(Milliseconds(500).toRawNanoseconds())
        || !sysTimer.set(Milliseconds(1000).toRawNanoseconds())) {
    LOGE("Couldn't set timer");
  } else {
    std::signal(SIGINT, signalHandler);
    runner.run();
  }

  return 0;
}
