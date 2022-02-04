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

#ifndef CHRE_RPC_SERVICE_MANAGER_H_
#define CHRE_RPC_SERVICE_MANAGER_H_

#include <cinttypes>
#include <cstdint>

#include <chre.h>

#include "chre/util/singleton.h"

namespace chre {
namespace rpc_service_test {

/**
 * Class to manage the CHRE rpc service nanoapp.
 */
class RpcServiceManager {
 public:
  /**
   * Allows the manager to do any init necessary as part of nanoappStart.
   */
  bool start();

  /**
   * Handle a CHRE event.
   *
   * @param senderInstanceId The instand ID that sent the event.
   * @param eventType The type of the event.
   * @param eventData The data for the event.
   */
  void handleEvent(uint32_t senderInstanceId, uint16_t eventType,
                   const void *eventData);
};

typedef chre::Singleton<RpcServiceManager> RpcServiceManagerSingleton;

}  // namespace rpc_service_test
}  // namespace chre

#endif  // CHRE_RPC_SERVICE_MANAGER_H_
