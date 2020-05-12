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

#ifndef CHRE_AUDIO_CONCURRENCY_TEST_MANAGER_H_
#define CHRE_AUDIO_CONCURRENCY_TEST_MANAGER_H_

#include <chre.h>
#include <cinttypes>

#include "chre/util/singleton.h"

namespace chre {

namespace audio_concurrency_test {

/**
 * A class to manage a CHRE audio concurrency test session.
 */
class Manager {
 public:
  enum class TestStep : uint8_t {
    ENABLE_AUDIO = 0,
    VERIFY_AUDIO_RESUME,
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
   * Initiates the test given a test command from the host.
   *
   * @param hostEndpointId The test host endpoint ID.
   * @param step The test step.
   *
   * @return true if the message was handled correctly.
   */
  bool handleTestCommandMessage(uint16_t hostEndpointId, TestStep step);
};

// The audio concurrency test manager singleton.
typedef chre::Singleton<Manager> ManagerSingleton;

}  // namespace audio_concurrency_test

}  // namespace chre

#endif  // CHRE_AUDIO_CONCURRENCY_TEST_MANAGER_H_
