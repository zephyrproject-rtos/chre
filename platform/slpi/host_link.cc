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

#include <inttypes.h>
#include <limits.h>

#include "qurt.h"

#include "chre/core/event_loop_manager.h"
#include "chre/core/host_comms_manager.h"
#include "chre/platform/log.h"
#include "chre/platform/shared/host_messages_generated.h"
#include "chre/platform/shared/host_protocol.h"
#include "chre/platform/slpi/fastrpc.h"
#include "chre/util/fixed_size_blocking_queue.h"

namespace chre {

namespace {

constexpr size_t kOutboundQueueSize = 32;
FixedSizeBlockingQueue<const MessageToHost *, kOutboundQueueSize>
    gOutboundQueue;

class MessageHandlers : public HostProtocol::IMessageHandlers {
 public:
  void handleNanoappMessage(
      uint64_t appId, uint32_t messageType, uint16_t hostEndpoint,
      const void *messageData, size_t messageDataLen) override {
    LOGD("Parsed nanoapp message from host: app ID 0x%016" PRIx64 ", endpoint "
         "0x%" PRIx16 ", msgType %" PRIu32 ", payload size %zu",
         appId, hostEndpoint, messageType, messageDataLen);

    HostCommsManager& manager =
        EventLoopManagerSingleton::get()->getHostCommsManager();
    manager.sendMessageToNanoappFromHost(
        appId, messageType, hostEndpoint, messageData, messageDataLen);
  }
};

/**
 * FastRPC method invoked by the host to block on messages
 *
 * @param buffer Output buffer to populate with message data
 * @param bufferLen Size of the buffer, in bytes
 * @param messageLen Output parameter to populate with the size of the message
 *        in bytes upon success
 *
 * @return 0 on success, nonzero on failure
 */
extern "C" int chre_slpi_get_message_to_host(
    unsigned char *buffer, int bufferLen, unsigned int *messageLen) {
  CHRE_ASSERT(buffer != nullptr);
  CHRE_ASSERT(bufferLen > 0);
  CHRE_ASSERT(messageLen != nullptr);

  const MessageToHost *msgToHost = gOutboundQueue.pop();
  int result;
  if (msgToHost == nullptr) {
    // A null message is used during shutdown so the calling thread can exit
    result = CHRE_FASTRPC_ERROR_SHUTTING_DOWN;
  } else {
    if (bufferLen <= 0
        || msgToHost->message.size() > INT_MAX
        || msgToHost->message.size() > static_cast<size_t>(bufferLen)) {
      // Note that we can't use regular logs here as they can result in sending
      // a message, leading to an infinite loop if the error is persistent
      FARF(FATAL, "Invalid buffer size %d or message size %zu", bufferLen,
           msgToHost->message.size());
      result = CHRE_FASTRPC_ERROR;
    } else {
      // TODO: ideally we'd construct our flatbuffer directly in the
      // host-supplied buffer
      constexpr size_t kInitialFlatBufferSize = 256;
      flatbuffers::FlatBufferBuilder builder(kInitialFlatBufferSize);
      HostProtocol::encodeNanoappMessage(
        builder, msgToHost->appId, msgToHost->toHostData.messageType,
        msgToHost->toHostData.hostEndpoint, msgToHost->message.data(),
        msgToHost->message.size());

      uint8_t *data = builder.GetBufferPointer();
      size_t size = builder.GetSize();
      if (size > bufferLen) {
        LOGE("Encoded structure size %zu too big for host buffer %d; dropping",
             size, bufferLen);
        CHRE_ASSERT(false);
        result = CHRE_FASTRPC_ERROR;
      } else {
        memcpy(buffer, data, size);
        *messageLen = size;
        result = CHRE_FASTRPC_SUCCESS;
      }
    }

    auto& hostCommsManager =
        EventLoopManagerSingleton::get()->getHostCommsManager();
    hostCommsManager.onMessageToHostComplete(msgToHost);
  }

  return result;
}

/**
 * FastRPC method invoked by the host to send a message to the system
 *
 * @param buffer
 * @param size
 *
 * @return 0 on success, nonzero on failure
 */
extern "C" int chre_slpi_deliver_message_from_host(const unsigned char *message,
                                                   int messageLen) {
  static MessageHandlers handlers;

  CHRE_ASSERT(message != nullptr);
  CHRE_ASSERT(messageLen > 0);
  int result = CHRE_FASTRPC_ERROR;

  if (message == nullptr || messageLen <= 0) {
    LOGE("Got null or invalid size (%d) message from host", messageLen);
  } else if (!HostProtocol::decodeMessage(message,
                                          static_cast<size_t>(messageLen),
                                          handlers)) {
    LOGE("Failed to decode/handle message");
  } else {
    result = CHRE_FASTRPC_SUCCESS;
  }

  return result;
}

}  // anonymous namespace

bool HostLink::sendMessage(const MessageToHost *message) {
  return gOutboundQueue.push(message);
}

void HostLinkBase::shutdown() {
  constexpr qurt_timer_duration_t kPollingIntervalUsec = 5000;

  // Push a null message so the blocking call in chre_slpi_get_message_to_host()
  // returns and the host can exit cleanly. If the queue is full, try again to
  // avoid getting stuck (no other new messages should be entering the queue at
  // this time). Don't wait too long as the host-side binary may have died in
  // a state where it's not blocked in chre_slpi_get_message_to_host().
  int retryCount = 5;
  FARF(MEDIUM, "Shutting down host link");
  while (!gOutboundQueue.push(nullptr) && --retryCount > 0) {
    qurt_timer_sleep(kPollingIntervalUsec);
  }

  if (retryCount <= 0) {
    // Don't use LOGE, as it may involve trying to send a message
    FARF(ERROR, "No room in outbound queue for shutdown message and host not "
         "draining queue!");
  } else {
    FARF(MEDIUM, "Draining message queue");

    // We were able to push the shutdown message. Wait for the queue to
    // completely flush before returning.
    int waitCount = 5;
    while (!gOutboundQueue.empty() && --waitCount > 0) {
      qurt_timer_sleep(kPollingIntervalUsec);
    }

    if (waitCount <= 0) {
      FARF(ERROR, "Host took too long to drain outbound queue; exiting anyway");
    } else {
      FARF(MEDIUM, "Finished draining queue");
    }
  }
}

}  // namespace chre
