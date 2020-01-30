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

#ifndef CHRE_SETTINGS_TEST_MANAGER_H_
#define CHRE_SETTINGS_TEST_MANAGER_H_

#include "chre_settings_test.nanopb.h"

#include <chre.h>
#include <cinttypes>

#include "chre/util/singleton.h"

namespace chre {

namespace settings_test {

/**
 * A class to manage a CHRE settings test session.
 */
class Manager {
 public:
  enum class Feature : uint8_t {
    WIFI_SCANNING = 0,
    WIFI_RTT,
    GNSS_LOCATION,
    GNSS_MEASUREMENT,
    WWAN_CELL_INFO,
  };

  enum class FeatureState : uint8_t {
    ENABLED = 0,
    DISABLED,
  };

  /**
   * Handles an event from CHRE. Semantics are the same as nanoappHandleEvent.
   */
  void handleEvent(uint32_t senderInstanceId, uint16_t eventType,
                   const void *eventData);

 private:
  /**
   * Handles a message from the host.
   *
   * @param senderInstanceId The sender instance ID of this message.
   * @param hostData The data from the host.
   */
  void handleMessageFromHost(uint32_t senderInstanceId,
                             const chreMessageFromHostData *hostData);

  /**
   * Initiates the test given a start command from the host.
   *
   * @param hostEndpointId The test host endpoint ID.
   * @param feature The feature to test.
   * @param state The feature state.
   */
  void handleStartTestMessage(uint16_t hostEndpointId, Feature feature,
                              FeatureState state);

  /**
   * Processes data from CHRE.
   *
   * @param eventType The event type as defined by CHRE.
   * @param eventData A pointer to the data.
   */
  void handleDataFromChre(uint16_t eventType, const void *eventData);
};

// The settings test manager singleton.
typedef chre::Singleton<Manager> ManagerSingleton;

}  // namespace settings_test

}  // namespace chre

#endif  // CHRE_SETTINGS_TEST_MANAGER_H_
