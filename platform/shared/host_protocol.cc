/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "chre/platform/shared/host_protocol.h"

#include <inttypes.h>

#include "chre/platform/shared/host_messages_generated.h"

#ifdef CHRE_HOST_BUILD
#include "chre_host/log.h"
#else
#include "chre/platform/log.h"
#endif

using flatbuffers::FlatBufferBuilder;

namespace chre {

void HostProtocol::encodeNanoappMessage(
    FlatBufferBuilder& builder, uint64_t appId, uint32_t messageType,
    uint16_t hostEndpoint, const void *messageData, size_t messageDataLen) {
  auto messageDataOffset = builder.CreateVector(
      static_cast<const uint8_t *>(messageData), messageDataLen);

  auto nanoappMessage = fbs::CreateNanoappMessage(
      builder, appId, messageType, hostEndpoint, messageDataOffset);
  auto container = fbs::CreateMessageContainer(
      builder, fbs::ChreMessage::NanoappMessage, nanoappMessage.Union());
  builder.Finish(container);
}

bool HostProtocol::decodeMessage(const void *message, size_t messageLen,
                                 IMessageHandlers& handlers) {
  bool success = false;

  if (message != nullptr) {
    flatbuffers::Verifier verifier(static_cast<const uint8_t *>(message),
                                   messageLen);
    if (!fbs::VerifyMessageContainerBuffer(verifier)) {
      LOGE("Got corrupted or invalid message (size %zu)", messageLen);
    } else {
      const fbs::MessageContainer *container =
          fbs::GetMessageContainer(message);

      success = true;
      switch (container->message_type()) {
        case fbs::ChreMessage::NanoappMessage: {
          const auto *nanoappMsg = static_cast<const fbs::NanoappMessage *>(
              container->message());
          // Required field; verifier ensures that this is not null (though it
          // may be empty)
          const flatbuffers::Vector<uint8_t> *msgData = nanoappMsg->message();
          handlers.handleNanoappMessage(
              nanoappMsg->app_id(), nanoappMsg->message_type(),
              nanoappMsg->host_endpoint(), msgData->data(), msgData->size());
          break;
        }

        default:
          LOGW("Got invalid/unexpected message type %" PRIu8,
               static_cast<uint8_t>(container->message_type()));
          success = false;
      }
    }
  }

  return success;
}

}  // namespace chre
