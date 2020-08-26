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

#include <gtest/gtest.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <thread>

#include "chpp/app.h"
#include "chpp/clients/loopback.h"
#include "chpp/macros.h"
#include "chpp/platform/log.h"
#include "chpp/transport.h"

namespace {

void *workThread(void *arg) {
  ChppTransportState *context = static_cast<ChppTransportState *>(arg);
  pthread_setname_np(pthread_self(), context->linkParams.workThreadName);

  chppWorkThreadStart(context);

  return nullptr;
}

/*
 * Test suite for the CHPP Loopback client/service
 */
class LoopbackTests : public testing::Test {
 protected:
  void SetUp() override {
    memset(&mClientTransportContext.linkParams, 0,
           sizeof(mClientTransportContext.linkParams));
    memset(&mServiceTransportContext.linkParams, 0,
           sizeof(mServiceTransportContext.linkParams));
    // The linkSendThread in the link layer is a link "to" the remote end.
    mServiceTransportContext.linkParams.linkThreadName = "Link to client";
    mServiceTransportContext.linkParams.workThreadName = "Service work";
    mClientTransportContext.linkParams.linkThreadName = "Link to service";
    mClientTransportContext.linkParams.workThreadName = "Client work";

    chppTransportInit(&mClientTransportContext, &mClientAppContext);
    chppAppInit(&mClientAppContext, &mClientTransportContext);

    chppTransportInit(&mServiceTransportContext, &mServiceAppContext);
    chppAppInit(&mServiceAppContext, &mServiceTransportContext);

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

  void TearDown() override {
    chppWorkThreadStop(&mClientTransportContext);
    pthread_join(mClientWorkThread, NULL);

    chppAppDeinit(&mClientAppContext);
    chppTransportDeinit(&mClientTransportContext);

    chppWorkThreadStop(&mServiceTransportContext);
    pthread_join(mServiceWorkThread, NULL);

    chppAppDeinit(&mServiceAppContext);
    chppTransportDeinit(&mServiceTransportContext);
  }

  ChppTransportState mClientTransportContext = {};
  ChppAppState mClientAppContext = {};

  ChppTransportState mServiceTransportContext = {};
  ChppAppState mServiceAppContext = {};

  pthread_t mClientWorkThread;
  pthread_t mServiceWorkThread;
};

TEST_F(LoopbackTests, SimpleStartStop) {
  // Simple test to make sure start/stop work threads work,
  // without crashes.
  std::this_thread::sleep_for(std::chrono::seconds(1));
}

TEST_F(LoopbackTests, SimpleLoopback) {
  // Wait for the reset to finish.
  std::this_thread::sleep_for(std::chrono::seconds(1));

  CHPP_LOGI("Starting loopback test ...");

  size_t testLen = 1000;
  uint8_t buf[testLen];
  for (size_t i = 0; i < testLen; i++) {
    buf[i] = (uint8_t)(i + 100);
  }

  struct ChppLoopbackTestResult result;

  result = chppRunLoopbackTest(&mClientAppContext, buf, testLen);
  EXPECT_EQ(result.error, CHPP_APP_ERROR_NONE);

  result = chppRunLoopbackTest(&mClientAppContext, buf, 10);
  EXPECT_EQ(result.error, CHPP_APP_ERROR_NONE);

  result = chppRunLoopbackTest(&mClientAppContext, buf, 1);
  EXPECT_EQ(result.error, CHPP_APP_ERROR_NONE);

  result = chppRunLoopbackTest(&mClientAppContext, buf, 0);
  EXPECT_EQ(result.error, CHPP_APP_ERROR_INVALID_LENGTH);
}

}  // namespace
