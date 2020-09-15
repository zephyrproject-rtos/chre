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

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <thread>

#include "app_test_base.h"
#include "chpp/app.h"
#include "chpp/clients/loopback.h"
#include "chpp/platform/log.h"

/*
 * Test suite for the CHPP Loopback client/service
 */
namespace chpp {
namespace {

TEST_F(AppTestBase, SimpleStartStop) {
  // Simple test to make sure start/stop work threads work,
  // without crashes.
}

TEST_F(AppTestBase, SimpleLoopback) {
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

}  //  anonymous namespace
}  //  namespace chpp
