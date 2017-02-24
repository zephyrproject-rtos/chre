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

#include "chre/core/host_comms_manager.h"

#include "chre/core/event_loop_manager.h"
#include "chre/platform/assert.h"
#include "chre/platform/context.h"
#include "chre/platform/host_link.h"

namespace chre {

bool HostCommsManager::sendMessageToHostFromCurrentNanoapp(
    void *messageData, uint32_t messageSize, uint32_t messageType,
    uint16_t hostEndpoint, chreMessageFreeFunction *freeCallback) {
  EventLoop *eventLoop = chre::getCurrentEventLoop();
  CHRE_ASSERT(eventLoop);

  Nanoapp *currentApp = eventLoop->getCurrentNanoapp();
  CHRE_ASSERT(currentApp);

  bool success = false;
  if (messageSize > 0 && messageData == nullptr) {
    LOGW("Rejecting malformed message (null data but non-zero size)");
  } else if (messageSize > CHRE_MESSAGE_TO_HOST_MAX_SIZE) {
    LOGW("Rejecting message of size %" PRIu32 " bytes (max %d)",
         messageSize, CHRE_MESSAGE_TO_HOST_MAX_SIZE);
  } else if (hostEndpoint == kHostEndpointUnspecified) {
    LOGW("Rejecting message to invalid host endpoint");
  } else {
    MessageToHost *msgToHost = mMessagePool.allocate();

    if (msgToHost == nullptr) {
      LOGE("Couldn't allocate messageData to host");
    } else {
      msgToHost->appId = currentApp->getAppId();
      msgToHost->instanceId = currentApp->getInstanceId();
      // TODO: add support for hostEndpoint in the CHRE API
      msgToHost->hostEndpoint = hostEndpoint;
      msgToHost->messageType = messageType;
      msgToHost->message.wrap(static_cast<uint8_t *>(messageData), messageSize);
      msgToHost->nanoappFreeFunction = freeCallback;

      success = mHostLink.sendMessage(msgToHost);
      if (!success) {
        freeMessageToHost(msgToHost);
      }
    }
  }

  return success;
}

void HostCommsManager::onMessageReceivedFromHost(
    uint64_t nanoappId, uint16_t hostEndpoint, uint32_t messageType,
    void *messageData, size_t messageSize) {
  // TODO: implement, also need to consider how to differentiate messages
  // intended for the CHRE system vs. those intended for nanoapps (likely use a
  // different callback for system messages)
}

void HostCommsManager::onMessageToHostComplete(const MessageToHost *message) {
  // Removing const on message since we own the memory and will deallocate it;
  // the caller (HostLink) only gets a const pointer
  auto *msgToHost = const_cast<MessageToHost *>(message);

  // If there's no free callback, we can free the message right away as the
  // message pool is thread-safe; otherwise, we need to do it from within the
  // EventLoop context.
  if (msgToHost->nanoappFreeFunction == nullptr) {
    mMessagePool.deallocate(msgToHost);
  } else {
    EventLoopManagerSingleton::get()->deferCallback(
        SystemCallbackType::MessageToHostComplete, msgToHost,
        onMessageToHostCompleteCallback);
  }
}

void HostCommsManager::freeMessageToHost(MessageToHost *msgToHost) {
  if (msgToHost->nanoappFreeFunction != nullptr) {
    msgToHost->nanoappFreeFunction(msgToHost->message.data(),
                                   msgToHost->message.size());
  }
  mMessagePool.deallocate(msgToHost);
}

void HostCommsManager::onMessageToHostCompleteCallback(
    uint16_t /*type*/, void *data) {
  EventLoopManagerSingleton::get()->getHostCommsManager().freeMessageToHost(
      static_cast<MessageToHost *>(data));
}

}  // namespace chre
