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

#include "chre_api/chre/gnss.h"
#include <cstdint>
#include "chre/core/event_loop_manager.h"
#include "chre/core/settings.h"
#include "chre/platform/linux/pal_gnss.h"
#include "chre/platform/log.h"
#include "chre/util/system/napp_permissions.h"
#include "chre_api/chre/event.h"

#include "gtest/gtest.h"
#include "inc/test_util.h"
#include "test_base.h"
#include "test_event.h"
#include "test_event_queue.h"
#include "test_util.h"

namespace chre {
namespace {

TEST_F(TestBase, GnssCanSubscribeAndUnsubscribeToLocation) {
  CREATE_CHRE_TEST_EVENT(LOCATION_REQUEST, 0);

  struct LocationRequest {
    bool enable;
    uint32_t cookie;
  };

  struct App : public TestNanoapp {
    uint32_t perms = NanoappPermissions::CHRE_PERMS_GNSS;

    void (*handleEvent)(uint32_t, uint16_t,
                        const void *) = [](uint32_t, uint16_t eventType,
                                           const void *eventData) {
      static uint32_t cookie;
      switch (eventType) {
        case CHRE_EVENT_GNSS_ASYNC_RESULT: {
          auto *event = static_cast<const chreAsyncResult *>(eventData);
          if (event->success) {
            TestEventQueueSingleton::get()->pushEvent(
                CHRE_EVENT_GNSS_ASYNC_RESULT,
                *(static_cast<const uint32_t *>(event->cookie)));
          }
          break;
        }

        case CHRE_EVENT_TEST_EVENT: {
          auto event = static_cast<const TestEvent *>(eventData);
          switch (event->type) {
            case LOCATION_REQUEST: {
              auto request = static_cast<const LocationRequest *>(event->data);
              if (request->enable) {
                cookie = request->cookie;
                chreGnssLocationSessionStartAsync(1000 /*minIntervalMs*/,
                                                  1000 /*minTimeToNextFixMs*/,
                                                  &cookie);
              } else {
                cookie = request->cookie;
                chreGnssLocationSessionStopAsync(&cookie);
              }
              break;
            }
          }
        }
      }
    };
  };

  auto app = loadNanoapp<App>();
  EXPECT_FALSE(chrePalGnssIsLocationEnabled());

  LocationRequest request{.enable = true, .cookie = 0x123};
  sendEventToNanoapp(app, LOCATION_REQUEST, request);
  uint32_t cookie;
  waitForEvent(CHRE_EVENT_GNSS_ASYNC_RESULT, &cookie);
  EXPECT_EQ(cookie, request.cookie);
  EXPECT_TRUE(chrePalGnssIsLocationEnabled());

  request.enable = false;
  sendEventToNanoapp(app, LOCATION_REQUEST, request);
  waitForEvent(CHRE_EVENT_GNSS_ASYNC_RESULT, &cookie);
  EXPECT_EQ(cookie, request.cookie);
  EXPECT_FALSE(chrePalGnssIsLocationEnabled());
}

TEST_F(TestBase, GnssUnsubscribeToLocationOnUnload) {
  CREATE_CHRE_TEST_EVENT(LOCATION_REQUEST, 0);

  struct LocationRequest {
    bool enable;
    uint32_t cookie;
  };

  struct App : public TestNanoapp {
    uint32_t perms = NanoappPermissions::CHRE_PERMS_GNSS;

    void (*handleEvent)(uint32_t, uint16_t,
                        const void *) = [](uint32_t, uint16_t eventType,
                                           const void *eventData) {
      static uint32_t cookie;
      switch (eventType) {
        case CHRE_EVENT_GNSS_ASYNC_RESULT: {
          auto *event = static_cast<const chreAsyncResult *>(eventData);
          if (event->success) {
            TestEventQueueSingleton::get()->pushEvent(
                CHRE_EVENT_GNSS_ASYNC_RESULT,
                *(static_cast<const uint32_t *>(event->cookie)));
          }
          break;
        }

        case CHRE_EVENT_TEST_EVENT: {
          auto event = static_cast<const TestEvent *>(eventData);
          switch (event->type) {
            case LOCATION_REQUEST: {
              auto request = static_cast<const LocationRequest *>(event->data);
              if (request->enable) {
                cookie = request->cookie;
                chreGnssLocationSessionStartAsync(1000 /*minIntervalMs*/,
                                                  1000 /*minTimeToNextFixMs*/,
                                                  &cookie);
              }
              break;
            }
          }
        }
      }
    };
  };

  auto app = loadNanoapp<App>();
  EXPECT_FALSE(chrePalGnssIsLocationEnabled());

  LocationRequest request{.enable = true, .cookie = 0x123};
  sendEventToNanoapp(app, LOCATION_REQUEST, request);
  uint32_t cookie;
  waitForEvent(CHRE_EVENT_GNSS_ASYNC_RESULT, &cookie);
  EXPECT_EQ(cookie, request.cookie);
  EXPECT_TRUE(chrePalGnssIsLocationEnabled());

  unloadNanoapp(app);
  EXPECT_FALSE(chrePalGnssIsLocationEnabled());
}

TEST_F(TestBase, GnssCanSubscribeAndUnsubscribeToMeasurement) {
  CREATE_CHRE_TEST_EVENT(MEASUREMENT_REQUEST, 0);

  struct MeasurementRequest {
    bool enable;
    uint32_t cookie;
  };

  struct App : public TestNanoapp {
    uint32_t perms = NanoappPermissions::CHRE_PERMS_GNSS;

    void (*handleEvent)(uint32_t, uint16_t, const void *) =
        [](uint32_t, uint16_t eventType, const void *eventData) {
          static uint32_t cookie;
          switch (eventType) {
            case CHRE_EVENT_GNSS_ASYNC_RESULT: {
              auto *event = static_cast<const chreAsyncResult *>(eventData);
              if (event->success) {
                TestEventQueueSingleton::get()->pushEvent(
                    CHRE_EVENT_GNSS_ASYNC_RESULT,
                    *(static_cast<const uint32_t *>(event->cookie)));
              }
              break;
            }

            case CHRE_EVENT_TEST_EVENT: {
              auto event = static_cast<const TestEvent *>(eventData);
              switch (event->type) {
                case MEASUREMENT_REQUEST: {
                  auto request =
                      static_cast<const MeasurementRequest *>(event->data);
                  if (request->enable) {
                    cookie = request->cookie;
                    chreGnssMeasurementSessionStartAsync(1000 /*minIntervalMs*/,
                                                         &cookie);
                  } else {
                    cookie = request->cookie;
                    chreGnssMeasurementSessionStopAsync(&cookie);
                  }
                  break;
                }
              }
            }
          }
        };
  };

  auto app = loadNanoapp<App>();
  EXPECT_FALSE(chrePalGnssIsLocationEnabled());

  MeasurementRequest request{.enable = true, .cookie = 0x123};
  sendEventToNanoapp(app, MEASUREMENT_REQUEST, request);
  uint32_t cookie;
  waitForEvent(CHRE_EVENT_GNSS_ASYNC_RESULT, &cookie);
  EXPECT_EQ(cookie, request.cookie);
  EXPECT_TRUE(chrePalGnssIsMeasurementEnabled());

  request.enable = false;
  sendEventToNanoapp(app, MEASUREMENT_REQUEST, request);
  waitForEvent(CHRE_EVENT_GNSS_ASYNC_RESULT, &cookie);
  EXPECT_EQ(cookie, request.cookie);
  EXPECT_FALSE(chrePalGnssIsMeasurementEnabled());
}

TEST_F(TestBase, GnssUnsubscribeToMeasurementOnUnload) {
  CREATE_CHRE_TEST_EVENT(MEASUREMENT_REQUEST, 0);

  struct MeasurementRequest {
    bool enable;
    uint32_t cookie;
  };

  struct App : public TestNanoapp {
    uint32_t perms = NanoappPermissions::CHRE_PERMS_GNSS;

    void (*handleEvent)(uint32_t, uint16_t, const void *) =
        [](uint32_t, uint16_t eventType, const void *eventData) {
          static uint32_t cookie;
          switch (eventType) {
            case CHRE_EVENT_GNSS_ASYNC_RESULT: {
              auto *event = static_cast<const chreAsyncResult *>(eventData);
              if (event->success) {
                TestEventQueueSingleton::get()->pushEvent(
                    CHRE_EVENT_GNSS_ASYNC_RESULT,
                    *(static_cast<const uint32_t *>(event->cookie)));
              }
              break;
            }

            case CHRE_EVENT_TEST_EVENT: {
              auto event = static_cast<const TestEvent *>(eventData);
              switch (event->type) {
                case MEASUREMENT_REQUEST: {
                  auto request =
                      static_cast<const MeasurementRequest *>(event->data);
                  if (request->enable) {
                    cookie = request->cookie;
                    chreGnssMeasurementSessionStartAsync(1000 /*minIntervalMs*/,
                                                         &cookie);
                  }
                  break;
                }
              }
            }
          };
        };
  };

  auto app = loadNanoapp<App>();
  EXPECT_FALSE(chrePalGnssIsLocationEnabled());

  MeasurementRequest request{.enable = true, .cookie = 0x123};
  sendEventToNanoapp(app, MEASUREMENT_REQUEST, request);
  uint32_t cookie;
  waitForEvent(CHRE_EVENT_GNSS_ASYNC_RESULT, &cookie);
  EXPECT_EQ(cookie, request.cookie);
  EXPECT_TRUE(chrePalGnssIsMeasurementEnabled());

  unloadNanoapp(app);
  EXPECT_FALSE(chrePalGnssIsMeasurementEnabled());
}

TEST_F(TestBase, GnssCanSubscribeAndUnsubscribeToPassiveListener) {
  CREATE_CHRE_TEST_EVENT(LISTENER_REQUEST, 0);

  struct App : public TestNanoapp {
    uint32_t perms = NanoappPermissions::CHRE_PERMS_GNSS;

    void (*handleEvent)(uint32_t, uint16_t, const void *) =
        [](uint32_t, uint16_t eventType, const void *eventData) {
          switch (eventType) {
            case CHRE_EVENT_TEST_EVENT: {
              auto event = static_cast<const TestEvent *>(eventData);
              switch (event->type) {
                case LISTENER_REQUEST: {
                  auto enable = *(static_cast<const bool *>(event->data));
                  chreGnssConfigurePassiveLocationListener(enable);
                  TestEventQueueSingleton::get()->pushEvent(LISTENER_REQUEST);
                  break;
                }
              }
            }
          }
        };
  };

  auto app = loadNanoapp<App>();
  EXPECT_FALSE(chrePalGnssIsPassiveLocationListenerEnabled());

  sendEventToNanoapp(app, LISTENER_REQUEST, true);
  waitForEvent(LISTENER_REQUEST);
  EXPECT_TRUE(chrePalGnssIsPassiveLocationListenerEnabled());

  sendEventToNanoapp(app, LISTENER_REQUEST, false);
  waitForEvent(LISTENER_REQUEST);
  EXPECT_FALSE(chrePalGnssIsPassiveLocationListenerEnabled());
}

TEST_F(TestBase, GnssUnsubscribeToPassiveListenerOnUnload) {
  CREATE_CHRE_TEST_EVENT(LISTENER_REQUEST, 0);

  struct App : public TestNanoapp {
    uint32_t perms = NanoappPermissions::CHRE_PERMS_GNSS;

    void (*handleEvent)(uint32_t, uint16_t, const void *) =
        [](uint32_t, uint16_t eventType, const void *eventData) {
          switch (eventType) {
            case CHRE_EVENT_TEST_EVENT: {
              auto event = static_cast<const TestEvent *>(eventData);
              switch (event->type) {
                case LISTENER_REQUEST: {
                  auto enable = *(static_cast<const bool *>(event->data));
                  chreGnssConfigurePassiveLocationListener(enable);
                  TestEventQueueSingleton::get()->pushEvent(LISTENER_REQUEST);
                }
              }
            }
          }
        };
  };

  auto app = loadNanoapp<App>();
  EXPECT_FALSE(chrePalGnssIsPassiveLocationListenerEnabled());

  sendEventToNanoapp(app, LISTENER_REQUEST, true);
  waitForEvent(LISTENER_REQUEST);
  EXPECT_TRUE(chrePalGnssIsPassiveLocationListenerEnabled());

  unloadNanoapp(app);
  EXPECT_FALSE(chrePalGnssIsPassiveLocationListenerEnabled());
}

}  // namespace
}  // namespace chre