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

#include <chre.h>
#include <cinttypes>

#include "chre/util/nanoapp/log.h"

#define LOG_TAG "[RpcServiceTest]"

namespace chre {

extern "C" void nanoappHandleEvent(uint32_t senderInstanceId,
                                   uint16_t eventType, const void *eventData) {
  LOGW("Got unknown event type from senderInstanceId %" PRIu32
       " and with eventType %" PRIu16,
       senderInstanceId, eventType);
}

extern "C" bool nanoappStart(void) {
  static chreNanoappRpcService sRpcService = {
      .id = 0xca8f7150a3f05847,
      .version = 0x01020034,
  };

  return chrePublishRpcServices(&sRpcService, 1 /* numServices */);
}

extern "C" void nanoappEnd(void) {}

}  // namespace chre
