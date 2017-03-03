/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <cinttypes>

#include "chre.h"

namespace chre {
namespace app {

namespace {

uint8_t gMessageData[] = {1, 2, 3, 4, 5, 6, 7, 8};

void messageFreeCallback(void *message, size_t messageSize) {
  chreLog(CHRE_LOG_INFO, "Message world got message free callback for message @"
          " %p (expected %d) size %zu (expected %d)",
          message, (message == gMessageData),
          messageSize, (messageSize == sizeof(gMessageData)));
}

}  // anonymous namespace

bool messageWorldStart() {
  chreLog(CHRE_LOG_INFO, "Message world app started as instance %" PRIu32,
          chreGetInstanceId());
  constexpr uint32_t kMessageType = 1234;

  bool success = chreSendMessageToHostEndpoint(
      gMessageData, sizeof(gMessageData), kMessageType,
      CHRE_HOST_ENDPOINT_BROADCAST, messageFreeCallback);
  chreLog(CHRE_LOG_INFO, "Sent message to host from start callback, result %d",
          success);

  return true;
}

void messageWorldHandleEvent(uint32_t senderInstanceId,
                             uint16_t eventType,
                             const void *eventData) {
  chreLog(CHRE_LOG_INFO, "Message world got event 0x%" PRIx16 " from instance "
          "%" PRIu32, eventType, senderInstanceId);

  if (eventType == CHRE_EVENT_MESSAGE_FROM_HOST) {
    auto *msg = static_cast<const chreMessageFromHostData *>(eventData);
    chreLog(CHRE_LOG_INFO, "Message world got message from host with type %"
            PRIu32 " size %" PRIu32 " data @ %p hostEndpoint 0x%" PRIx16,
            msg->messageType, msg->messageSize, msg->message,
            msg->hostEndpoint);
    if (senderInstanceId != CHRE_INSTANCE_ID) {
      chreLog(CHRE_LOG_ERROR, "Message from host came from unexpected instance "
              "ID %" PRIu32, senderInstanceId);
    }
  }
}

void messageWorldStop() {
  chreLog(CHRE_LOG_INFO, "Message world app started");
}

}  // namespace app
}  // namespace chre
