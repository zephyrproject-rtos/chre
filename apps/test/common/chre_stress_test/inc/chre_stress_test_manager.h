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

#ifndef CHRE_STRESS_TEST_MANAGER_H_
#define CHRE_STRESS_TEST_MANAGER_H_

#include "chre_stress_test.nanopb.h"

#include <chre.h>
#include <cinttypes>

#include "chre/util/singleton.h"

namespace chre {

namespace stress_test {

/**
 * A class to manage a CHRE stress test session.
 */
class Manager {
 public:
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
};

// The stress test manager singleton.
typedef chre::Singleton<Manager> ManagerSingleton;

}  // namespace stress_test

}  // namespace chre

#endif  // CHRE_STRESS_TEST_MANAGER_H_
