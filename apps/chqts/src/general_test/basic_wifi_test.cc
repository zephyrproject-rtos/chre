/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <general_test/basic_wifi_test.h>

#include <chre.h>
#include <shared/send_message.h>

/*
 * Test to check expected functionality of the CHRE WiFi APIs.
 *
 * 1. If scan monitor is not supported, skip to 5;
 *    otherwise enables scan monitor.
 * 2. Checks async result of enabling scan monitor.
 * 3. Disables scan monitor.
 * 4. Checks async result of disabling scan monitor.
 * 5. If on demand WiFi scan is not supported, skip to end;
 *    otherwise sends default scan request.
 * 6. Checks the result of on demand WiFi scan.
 */
namespace general_test {

namespace {

//! A dummy cookie to pass into the enable
//! configure scan monitoring async request.
constexpr uint32_t kEnableScanMonitoringCookie = 0x1337;

//! A dummy cookie to pass into the disable
//! configure scan monitoring async request.
constexpr uint32_t kDisableScanMonitoringCookie = 0x1338;

//! A dummy cookie to pass into request scan async.
constexpr uint32_t kOnDemandScanCookie = 0xcafe;

/**
 * Calls API testConfigureScanMonitorAsync. Sends fatal failure to host
 * if API call fails.
 *
 * @param enable Set to true to enable monitoring scan results,
 *        false to disable.
 * @param cookie An opaque value that will be included in the chreAsyncResult
 *        sent in relation to this request.
 */
void testConfigureScanMonitorAsync(bool enable, const void * cookie) {
  if (!chreWifiConfigureScanMonitorAsync(enable, cookie)) {
    if (enable) {
      nanoapp_testing::sendFatalFailureToHost(
          "Failed to request to enable scan monitor.");
    } else {
      nanoapp_testing::sendFatalFailureToHost(
          "Failed to request to disable scan monitor.");
    }
  }
}

/**
 * Calls API chreWifiRequestScanAsyncDefault. Sends fatal failure to host
 * if API call fails.
 */
void testRequestScanAsync() {
  if (!chreWifiRequestScanAsyncDefault(&kOnDemandScanCookie)) {
    nanoapp_testing::sendFatalFailureToHost(
        "Failed to request for on-demand WiFi scan.");
  }
}

} // anonymous namespace

BasicWifiTest::BasicWifiTest()
    : Test(CHRE_API_VERSION_1_1) {
}

void BasicWifiTest::setUp(
    uint32_t messageSize, const void * /* message */) {
  if (messageSize != 0) {
    nanoapp_testing::sendFatalFailureToHost(
        "Expected 0 byte message, got more bytes:", &messageSize);
  } else {
    mWifiCapabilities = chreWifiGetCapabilities();
    startScanMonitorTestStage();
  }
}

void BasicWifiTest::handleEvent(uint32_t /* senderInstanceId */,
                                uint16_t eventType,
                                const void *eventData) {
  if (eventData == nullptr) {
    nanoapp_testing::sendFatalFailureToHost(
        "Received null eventData");
  }
  switch (eventType) {
    case CHRE_EVENT_WIFI_ASYNC_RESULT:
      handleChreWifiAsyncEvent(eventData);
      break;
    case CHRE_EVENT_WIFI_SCAN_RESULT:
      if (isActiveWifiScanType(eventData)) {
        // TODO (jacksun) remove the follwing line,
        // and add validating wifi scan results.
        mTestSuccessMarker.markStageAndSuccessOnFinish(
            BASIC_WIFI_TEST_STAGE_SCAN_ASYNC);
      }
      break;
    default:
      unexpectedEvent(eventType);
      break;
  }
}

void BasicWifiTest::handleChreWifiAsyncEvent(const void *eventData) {
  if (!mCurrentWifiRequest.has_value()) {
    nanoapp_testing::sendFailureToHost("Unexpected async result");
  }
  const auto *result =
    static_cast<const chreAsyncResult *>(eventData);
  validateChreAsyncResult(result, mCurrentWifiRequest.value());

  if (result->requestType == CHRE_WIFI_REQUEST_TYPE_CONFIGURE_SCAN_MONITOR) {
    if (mCurrentWifiRequest->cookie == &kDisableScanMonitoringCookie) {
      mTestSuccessMarker.markStageAndSuccessOnFinish(
          BASIC_WIFI_TEST_STAGE_SCAN_MONITOR);
      startScanAsyncTestStage();
    } else {
      testConfigureScanMonitorAsync(false /* enable */,
                                    &kDisableScanMonitoringCookie);
      resetCurrentWifiRequest(&kDisableScanMonitoringCookie,
                              CHRE_WIFI_REQUEST_TYPE_CONFIGURE_SCAN_MONITOR,
                              CHRE_ASYNC_RESULT_TIMEOUT_NS);
    }
  }
}

bool BasicWifiTest::isActiveWifiScanType(const void *eventData) {
  const auto *result =
      static_cast<const chreWifiScanEvent *>(eventData);
  return (result->scanType == CHRE_WIFI_SCAN_TYPE_ACTIVE);
}

void BasicWifiTest::startScanMonitorTestStage() {
  if (mWifiCapabilities & CHRE_WIFI_CAPABILITIES_SCAN_MONITORING) {
    testConfigureScanMonitorAsync(true /* enable */,
                                  &kEnableScanMonitoringCookie);
    resetCurrentWifiRequest(&kEnableScanMonitoringCookie,
                            CHRE_WIFI_REQUEST_TYPE_CONFIGURE_SCAN_MONITOR,
                            CHRE_ASYNC_RESULT_TIMEOUT_NS);
  } else {
    mTestSuccessMarker.markStageAndSuccessOnFinish(
        BASIC_WIFI_TEST_STAGE_SCAN_MONITOR);
    startScanAsyncTestStage();
  }
}

void BasicWifiTest::startScanAsyncTestStage() {
  if (mWifiCapabilities & CHRE_WIFI_CAPABILITIES_ON_DEMAND_SCAN) {
    testRequestScanAsync();
    resetCurrentWifiRequest(&kOnDemandScanCookie,
                            CHRE_WIFI_REQUEST_TYPE_REQUEST_SCAN,
                            CHRE_WIFI_SCAN_RESULT_TIMEOUT_NS);
  } else {
    mTestSuccessMarker.markStageAndSuccessOnFinish(
        BASIC_WIFI_TEST_STAGE_SCAN_ASYNC);
  }
}

void BasicWifiTest::resetCurrentWifiRequest(const void *cookie,
                                            uint8_t requstType,
                                            uint64_t timeoutNs) {
  chreAsyncRequest request = {
    .cookie = cookie,
    .requestType = requstType,
    .requestTimeNs = chreGetTime(),
    .timeoutNs = timeoutNs
  };
  mCurrentWifiRequest = request;
}

} // namespace general_test
