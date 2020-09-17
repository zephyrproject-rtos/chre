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
  // Simple test to make sure start/stop work threads work without crashing
}

TEST_F(AppTestBase, SimpleLoopback) {
  constexpr size_t kTestLen =
      CHPP_TRANSPORT_TX_MTU_BYTES - CHPP_LOOPBACK_HEADER_LEN;
  uint8_t buf[kTestLen];
  for (size_t i = 0; i < kTestLen; i++) {
    buf[i] = (uint8_t)(i + 100);
  }

  CHPP_LOGI(
      "Starting loopback test without fragmentation (max buffer = %zu)...",
      kTestLen);

  struct ChppLoopbackTestResult result;

  result = chppRunLoopbackTest(&mClientAppContext, buf, kTestLen);
  EXPECT_EQ(result.error, CHPP_APP_ERROR_NONE);

  result = chppRunLoopbackTest(&mClientAppContext, buf, 10);
  EXPECT_EQ(result.error, CHPP_APP_ERROR_NONE);

  result = chppRunLoopbackTest(&mClientAppContext, buf, 1);
  EXPECT_EQ(result.error, CHPP_APP_ERROR_NONE);

  result = chppRunLoopbackTest(&mClientAppContext, buf, 0);
  EXPECT_EQ(result.error, CHPP_APP_ERROR_INVALID_LENGTH);
}

TEST_F(AppTestBase, FragmentedLoopback) {
  constexpr size_t kTestLen = UINT16_MAX;
  uint8_t buf[kTestLen];
  for (size_t i = 0; i < kTestLen; i++) {
    buf[i] = (uint8_t)(i + 100);
  }

  CHPP_LOGI("Starting loopback test with fragmentation (max buffer = %zu)...",
            kTestLen);

  struct ChppLoopbackTestResult result;

  result = chppRunLoopbackTest(&mClientAppContext, buf, kTestLen);
  EXPECT_EQ(result.error, CHPP_APP_ERROR_NONE);

  result = chppRunLoopbackTest(&mClientAppContext, buf, 50000);
  EXPECT_EQ(result.error, CHPP_APP_ERROR_NONE);

  result = chppRunLoopbackTest(
      &mClientAppContext, buf,
      CHPP_TRANSPORT_TX_MTU_BYTES - CHPP_LOOPBACK_HEADER_LEN + 1);
  EXPECT_EQ(result.error, CHPP_APP_ERROR_NONE);
}

}  // namespace
}  // namespace chpp
