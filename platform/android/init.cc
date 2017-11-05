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

#include "chre/core/event_loop_manager.h"
#include "chre/core/init.h"
#include "chre/core/static_nanoapps.h"
#include "chre/platform/shared/platform_log.h"

#include <csignal>
#include <thread>

using chre::EventLoopManagerSingleton;

namespace {

extern "C" void signalHandler(int sig) {
  (void) sig;
  LOGI("Stop request received");
  EventLoopManagerSingleton::get()->getEventLoop().stop();
}

}

int main(int argc, char **argv) {
  // Initilize the system.
  chre::init();

  // Register a signal handler and start CHRE.
  std::signal(SIGINT, signalHandler);
  chre::loadStaticNanoapps();
  EventLoopManagerSingleton::get()->getEventLoop().run();

  chre::deinit();
  return 0;
}
