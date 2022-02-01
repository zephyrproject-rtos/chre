/*
 * Copyright (C) 2022 The Android Open Source Project
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
#include "chre/core/settings.h"
#include "chre/platform/linux/pal_nan.h"
#include "chre/platform/log.h"
#include "chre/util/system/napp_permissions.h"
#include "chre_api/chre/event.h"
#include "chre_api/chre/wifi.h"

#include "test_base.h"
#include "test_event_queue.h"
#include "test_util.h"

/**
 * Simulation to test WiFi NAN functionality in CHRE.
 *
 * The test works as follows:
 * - A test nanoapp starts by requesting NAN subscriptions, with random
 *   service specific information. It also requests NAN ranging measurements
 *   if the test desires it. The Linux WiFi PAL has hooks and flags that
 *   instruct it to cover various test cases (fail subscribe, terminate
 *   service, etc.), to enable testing of all NAN events that CHRE is
 *   expected to propagate. These flags should be set before startTestNanoapping
 * the test nanoapp.
 *
 * - The test fails (times out) if any of the events are not sent by CHRE.
 */

#define CHRE_EVENT_SIMULATION_NAN(offset) \
  CHRE_SPECIFIC_SIMULATION_TEST_EVENT_ID(offset)

#define CHRE_NAN_TEST_EVENT_IDENTIFIER_SUCCESS CHRE_EVENT_SIMULATION_NAN(0)

#define CHRE_NAN_TEST_EVENT_IDENTIFIER_FAILURE CHRE_EVENT_SIMULATION_NAN(1)

#define CHRE_NAN_TEST_EVENT_SERVICE_DISCOVERED CHRE_EVENT_SIMULATION_NAN(2)

#define CHRE_NAN_TEST_EVENT_ASYNC_ERROR CHRE_EVENT_SIMULATION_NAN(3)

#define CHRE_NAN_TEST_EVENT_SERVICE_TERMINATED CHRE_EVENT_SIMULATION_NAN(4)

#define CHRE_NAN_TEST_EVENT_SERVICE_LOST CHRE_EVENT_SIMULATION_NAN(5)

#define CHRE_NAN_TEST_EVENT_RANGING_REQUEST_SUCCESSFUL \
  CHRE_EVENT_SIMULATION_NAN(4)

#define CHRE_NAN_TEST_EVENT_RANGING_RESULT CHRE_EVENT_SIMULATION_NAN(7)

namespace chre {
namespace {

constexpr uint32_t kSubscribeCookie = 0x10aded;
constexpr uint32_t kRangingCookie = 0xfa11;

bool gRequestNanRanging = false;
uint8_t gExpectedErrorCode = CHRE_ERROR_NONE;
uint32_t gSubscriptionId = 0;
uint32_t gPublishId = 0;

bool start() {
  LOGD("WiFi NAN test nanoapp started");
  bool success = false;

  chreWifiNanSubscribeConfig config = {
      .subscribeType = CHRE_WIFI_NAN_SUBSCRIBE_TYPE_PASSIVE,
      .service = "SomeServiceName",
  };

  success = chreWifiNanSubscribe(&config, &kSubscribeCookie);

  if (success && gRequestNanRanging) {
    uint8_t fakeMacAddress[CHRE_WIFI_BSSID_LEN] = {0x1, 0x2, 0x3,
                                                   0x4, 0x5, 0x6};
    struct chreWifiNanRangingParams fakeRangingParams;
    std::memcpy(fakeRangingParams.macAddress, fakeMacAddress,
                CHRE_WIFI_BSSID_LEN);
    success =
        chreWifiNanRequestRangingAsync(&fakeRangingParams, &kRangingCookie);
  }
  return success;
}

void handleIdentifierEvent(const chreWifiNanIdentifierEvent *event) {
  auto &result = event->result;
  auto eventId = CHRE_NAN_TEST_EVENT_IDENTIFIER_FAILURE;

  EXPECT_TRUE(event != nullptr);

  if (result.errorCode == CHRE_ERROR_NONE) {
    gSubscriptionId = event->id;
    eventId = CHRE_NAN_TEST_EVENT_IDENTIFIER_SUCCESS;
  }
  TestEventQueueSingleton::get()->pushEvent(eventId);
}

void handleDiscoveryEvent(const chreWifiNanDiscoveryEvent *event) {
  EXPECT_TRUE(event != nullptr);
  EXPECT_EQ(gSubscriptionId, event->subscribeId);
  gPublishId = event->publishId;
  TestEventQueueSingleton::get()->pushEvent(
      CHRE_NAN_TEST_EVENT_SERVICE_DISCOVERED);
}

void handleTerminationEvent(const chreWifiNanSessionTerminatedEvent *event) {
  EXPECT_EQ(gSubscriptionId, event->id);
  TestEventQueueSingleton::get()->pushEvent(
      CHRE_NAN_TEST_EVENT_SERVICE_TERMINATED);
}

void handleSessionLostEvent(const chreWifiNanSessionLostEvent *event) {
  EXPECT_TRUE(event != nullptr);
  EXPECT_EQ(gSubscriptionId, event->id);
  EXPECT_EQ(gPublishId, event->peerId);
  TestEventQueueSingleton::get()->pushEvent(CHRE_NAN_TEST_EVENT_SERVICE_LOST);
}

void handleRangingResultEvent(const chreWifiRangingResult * /*event*/) {
  TestEventQueueSingleton::get()->pushEvent(CHRE_NAN_TEST_EVENT_RANGING_RESULT);
}

void handleAsyncEvent(const chreAsyncResult *event) {
  switch (event->requestType) {
    case CHRE_WIFI_REQUEST_TYPE_NAN_SUBSCRIBE: {
      EXPECT_EQ(event->errorCode, gExpectedErrorCode);
      TestEventQueueSingleton::get()->pushEvent(
          CHRE_NAN_TEST_EVENT_ASYNC_ERROR);
      break;
    }

    case CHRE_WIFI_REQUEST_TYPE_RANGING: {
      TestEventQueueSingleton::get()->pushEvent(
          CHRE_NAN_TEST_EVENT_RANGING_REQUEST_SUCCESSFUL);
      break;
    }

    default:
      LOGE("Unknown async result event");
  }
}

void handleEvent(uint32_t /*senderInstanceId*/, uint16_t eventType,
                 const void *eventData) {
  switch (eventType) {
    case CHRE_EVENT_WIFI_NAN_IDENTIFIER_RESULT:
      handleIdentifierEvent(
          static_cast<const chreWifiNanIdentifierEvent *>(eventData));
      break;

    case CHRE_EVENT_WIFI_NAN_DISCOVERY_RESULT:
      handleDiscoveryEvent(
          static_cast<const chreWifiNanDiscoveryEvent *>(eventData));
      break;

    case CHRE_EVENT_WIFI_ASYNC_RESULT:
      handleAsyncEvent(static_cast<const chreAsyncResult *>(eventData));
      break;

    case CHRE_EVENT_WIFI_NAN_SESSION_TERMINATED:
      handleTerminationEvent(
          static_cast<const chreWifiNanSessionTerminatedEvent *>(eventData));
      break;

    case CHRE_EVENT_WIFI_NAN_SESSION_LOST:
      handleSessionLostEvent(
          static_cast<const chreWifiNanSessionLostEvent *>(eventData));
      break;

    case CHRE_EVENT_WIFI_RANGING_RESULT:
      handleRangingResultEvent(
          static_cast<const chreWifiRangingResult *>(eventData));
      break;

    default:
      EXPECT_EQ(0, eventType) << "Unexpected evt " << eventType << " received";
  }
}

void end() {}

void startTestNanoapp() {
  constexpr uint64_t kAppId = 0x0123456789abcdef;
  constexpr uint32_t kAppVersion = 0;
  constexpr uint32_t kAppPerms = NanoappPermissions::CHRE_PERMS_WIFI;

  UniquePtr<Nanoapp> nanoapp = createStaticNanoapp(
      "Test nanoapp", kAppId, kAppVersion, kAppPerms, start, handleEvent, end);

  EventLoopManagerSingleton::get()->deferCallback(
      SystemCallbackType::FinishLoadingNanoapp, std::move(nanoapp),
      testFinishLoadingNanoappCallback);
}

void reset() {
  gRequestNanRanging = false;
  gSubscriptionId = 0;
  gPublishId = 0;
  gExpectedErrorCode = CHRE_ERROR_NONE;
  EventLoopManagerSingleton::get()->getSettingManager().postSettingChange(
      Setting::WIFI_AVAILABLE, SettingState::ENABLED);
  PalNanEngineSingleton::get()->setFlags(PalNanEngine::Flags::NONE);
}

}  // anonymous namespace

/**
 * Test that an async error is received if NAN operations are attempted when
 * the WiFi setting is disabled.
 */
TEST_F(TestBase, WifiNanDisabledViaSettings) {
  EventLoopManagerSingleton::get()->getSettingManager().postSettingChange(
      Setting::WIFI_AVAILABLE, SettingState::DISABLED);
  gExpectedErrorCode = CHRE_ERROR_FUNCTION_DISABLED;
  startTestNanoapp();
  waitForEvent(CHRE_NAN_TEST_EVENT_ASYNC_ERROR);
  reset();
}

/**
 * Test that a subscription request succeeds, and an identifier event is
 * received with a matching cookie. Also test that a discovery event is later
 * received, marking the completion of the subscription process.
 */
TEST_F(TestBase, WifiNanSuccessfulSubscribeTest) {
  startTestNanoapp();
  waitForEvent(CHRE_NAN_TEST_EVENT_IDENTIFIER_SUCCESS);

  PalNanEngineSingleton::get()->sendDiscoveryEvent(gSubscriptionId);

  waitForEvent(CHRE_NAN_TEST_EVENT_SERVICE_DISCOVERED);
  reset();
}

/**
 * Test that a subscription request fails, and an identifier event is received
 * with a matching cookie, indicating the reason for the error (Note that the
 * fake PAL engine always returns the generic CHRE_ERROR as the error code, but
 * this may vary in unsimulated scenarios).
 */
TEST_F(TestBase, WifiNanUnuccessfulSubscribeTest) {
  PalNanEngineSingleton::get()->setFlags(PalNanEngine::Flags::FAIL_SUBSCRIBE);
  startTestNanoapp();
  waitForEvent(CHRE_NAN_TEST_EVENT_IDENTIFIER_FAILURE);
  reset();
}

/**
 * Test that a terminated event is received upon the Pal NAN engine terminating
 * a discovered service.
 */
TEST_F(TestBase, WifiNanServiceTerminatedTest) {
  startTestNanoapp();
  waitForEvent(CHRE_NAN_TEST_EVENT_IDENTIFIER_SUCCESS);

  PalNanEngineSingleton::get()->sendDiscoveryEvent(gSubscriptionId);
  waitForEvent(CHRE_NAN_TEST_EVENT_SERVICE_DISCOVERED);

  PalNanEngineSingleton::get()->onServiceTerminated(gSubscriptionId);
  waitForEvent(CHRE_NAN_TEST_EVENT_SERVICE_TERMINATED);
  reset();
}

/**
 * Test that a service lost event is received upon the Pal NAN engine 'losing'
 * a discovered service.
 */
TEST_F(TestBase, WifiNanServiceLostTest) {
  startTestNanoapp();
  waitForEvent(CHRE_NAN_TEST_EVENT_IDENTIFIER_SUCCESS);

  PalNanEngineSingleton::get()->sendDiscoveryEvent(gSubscriptionId);

  waitForEvent(CHRE_NAN_TEST_EVENT_SERVICE_DISCOVERED);

  PalNanEngineSingleton::get()->onServiceLost(gSubscriptionId, gPublishId);
  waitForEvent(CHRE_NAN_TEST_EVENT_SERVICE_LOST);
  reset();
}

/**
 * Test that a ranging event is received upon requesting NAN range
 * measurements.
 */
TEST_F(TestBase, WifiNanRangingTest) {
  gRequestNanRanging = true;
  startTestNanoapp();
  waitForEvent(CHRE_NAN_TEST_EVENT_RANGING_REQUEST_SUCCESSFUL);
  waitForEvent(CHRE_NAN_TEST_EVENT_RANGING_RESULT);
  reset();
}

}  // namespace chre
