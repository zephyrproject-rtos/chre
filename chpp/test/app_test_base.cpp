/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "app_test_base.h"

#include <gtest/gtest.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <thread>

#include "chpp/app.h"
#include "chpp/macros.h"
#include "chpp/transport.h"

namespace chpp {
namespace {

void *workThread(void *arg) {
  ChppTransportState *context = static_cast<ChppTransportState *>(arg);
  pthread_setname_np(pthread_self(), context->linkParams.workThreadName);

  chppWorkThreadStart(context);

  return nullptr;
}

}  // anonymous namespace

void AppTestBase::SetUp() {
  memset(&mClientTransportContext.linkParams, 0,
         sizeof(mClientTransportContext.linkParams));
  memset(&mServiceTransportContext.linkParams, 0,
         sizeof(mServiceTransportContext.linkParams));
  // The linkSendThread in the link layer is a link "to" the remote end.
  mServiceTransportContext.linkParams.linkThreadName = "Link to client";
  mServiceTransportContext.linkParams.workThreadName = "Service work";
  mClientTransportContext.linkParams.linkThreadName = "Link to service";
  mClientTransportContext.linkParams.workThreadName = "Client work";

  struct ChppClientServiceSet set;
  memset(&set, 0, sizeof(set));
  set.wifiClient = 1;
  set.gnssClient = 1;
  set.wwanClient = 1;
  set.loopbackClient = 1;

  chppTransportInit(&mClientTransportContext, &mClientAppContext);
  chppAppInitWithClientServiceSet(&mClientAppContext, &mClientTransportContext,
                                  set);

  memset(&set, 0, sizeof(set));
  set.wifiService = 1;
  set.gnssService = 1;
  set.wwanService = 1;
  chppTransportInit(&mServiceTransportContext, &mServiceAppContext);
  chppAppInitWithClientServiceSet(&mServiceAppContext,
                                  &mServiceTransportContext, set);

  mClientTransportContext.linkParams.remoteTransportContext =
      &mServiceTransportContext;
  mServiceTransportContext.linkParams.remoteTransportContext =
      &mClientTransportContext;

  pthread_create(&mClientWorkThread, NULL, workThread,
                 &mClientTransportContext);

  // Wait a bit to emulate the scenario where the remote is not yet up
  std::this_thread::sleep_for(std::chrono::seconds(1));
  pthread_create(&mServiceWorkThread, NULL, workThread,
                 &mServiceTransportContext);
  mClientTransportContext.linkParams.linkEstablished = true;
  mServiceTransportContext.linkParams.linkEstablished = true;
}

void AppTestBase::TearDown() {
  chppWorkThreadStop(&mClientTransportContext);
  pthread_join(mClientWorkThread, NULL);

  chppAppDeinit(&mClientAppContext);
  chppTransportDeinit(&mClientTransportContext);

  chppWorkThreadStop(&mServiceTransportContext);
  pthread_join(mServiceWorkThread, NULL);

  chppAppDeinit(&mServiceAppContext);
  chppTransportDeinit(&mServiceTransportContext);
}

}  // namespace chpp
