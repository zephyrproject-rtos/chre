/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "test_base.h"

#include <gtest/gtest.h>

#include <thread>

#include "chre/core/event_loop_manager.h"
#include "chre/core/settings.h"
#include "chre/platform/linux/pal_gnss.h"
#include "chre/platform/log.h"
#include "chre/util/system/napp_permissions.h"
#include "chre_api/chre/gnss.h"
#include "chre_api/chre/user_settings.h"
#include "test_event_queue.h"
#include "test_util.h"

namespace chre {

namespace {

int8_t gExpectedSettingState = CHRE_USER_SETTING_STATE_DISABLED;

bool start() {
  chreGnssLocationSessionStartAsync(50, 50, nullptr);
  chreUserSettingConfigureEvents(CHRE_USER_SETTING_LOCATION, true /* enable */);
  TestEventQueueSingleton::get()->pushEvent(
      CHRE_EVENT_SIMULATION_TEST_NANOAPP_LOADED);
  return true;
}

void handleEvent(uint32_t /* senderInstanceId */, uint16_t eventType,
                 const void *eventData) {
  if (eventType == CHRE_EVENT_SETTING_CHANGED_LOCATION) {
    auto *event = static_cast<const chreUserSettingChangedEvent *>(eventData);
    EXPECT_EQ(gExpectedSettingState, event->settingState);
  }

  TestEventQueueSingleton::get()->pushEvent(eventType);
}

void end() {
  chreUserSettingConfigureEvents(CHRE_USER_SETTING_LOCATION,
                                 false /* enable */);
}

}  // anonymous namespace

/**
 * This test verifies the following GNSS settings behavior:
 * 1) Nanoapp makes GNSS request
 * 2) Toggle location setting -> disabled
 * 3) Toggle location setting -> enabled.
 * 4) Verify things resume.
 */
TEST_F(TestBase, LocationSettingsTest) {
  constexpr uint64_t kAppId = 0x0123456789abcdef;
  constexpr uint32_t kAppVersion = 0;
  constexpr uint32_t kAppPerms = NanoappPermissions::CHRE_PERMS_GNSS;

  UniquePtr<Nanoapp> nanoapp = createStaticNanoapp(
      "Test nanoapp", kAppId, kAppVersion, kAppPerms, start, handleEvent, end);
  EventLoopManagerSingleton::get()->deferCallback(
      SystemCallbackType::FinishLoadingNanoapp, std::move(nanoapp),
      testFinishLoadingNanoappCallback);
  waitForEvent(CHRE_EVENT_SIMULATION_TEST_NANOAPP_LOADED);

  waitForEvent(CHRE_EVENT_GNSS_ASYNC_RESULT);
  ASSERT_TRUE(chrePalGnssIsLocationEnabled());
  waitForEvent(CHRE_EVENT_GNSS_LOCATION);

  gExpectedSettingState = CHRE_USER_SETTING_STATE_DISABLED;
  EventLoopManagerSingleton::get()->getSettingManager().postSettingChange(
      Setting::LOCATION, false /* enabled */);
  waitForEvent(CHRE_EVENT_SETTING_CHANGED_LOCATION);
  ASSERT_EQ(
      EventLoopManagerSingleton::get()->getSettingManager().getSettingEnabled(
          Setting::LOCATION),
      false);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_FALSE(chrePalGnssIsLocationEnabled());

  gExpectedSettingState = CHRE_USER_SETTING_STATE_ENABLED;
  EventLoopManagerSingleton::get()->getSettingManager().postSettingChange(
      Setting::LOCATION, true /* enabled */);
  waitForEvent(CHRE_EVENT_SETTING_CHANGED_LOCATION);
  ASSERT_EQ(
      EventLoopManagerSingleton::get()->getSettingManager().getSettingEnabled(
          Setting::LOCATION),
      true);

  waitForEvent(CHRE_EVENT_GNSS_LOCATION);
  ASSERT_TRUE(chrePalGnssIsLocationEnabled());
}

TEST_F(TestBase, DefaultSettingsAreSet) {
  for (uint8_t setting = CHRE_USER_SETTING_LOCATION;
       setting <= CHRE_USER_SETTING_BLE_AVAILABLE; ++setting) {
    int8_t expectedSettingState = (setting == CHRE_USER_SETTING_AIRPLANE_MODE)
                                      ? CHRE_USER_SETTING_STATE_DISABLED
                                      : CHRE_USER_SETTING_STATE_ENABLED;
    EXPECT_EQ(expectedSettingState, chreUserSettingGetState(setting));
  }
}

}  // namespace chre
