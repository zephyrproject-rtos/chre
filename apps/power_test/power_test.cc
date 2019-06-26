/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include "chre_power_test_generated.h"
#include "chre/util/nanoapp/callbacks.h"
#include "common.h"
#include "request_manager.h"

#ifdef CHRE_NANOAPP_INTERNAL
namespace chre {
namespace {
#endif  // CHRE_NANOAPP_INTERNAL

using flatbuffers::FlatBufferBuilder;
using chre::power_test::MessageType;

/**
 * Responds to a host request indicating whether the request was successfully
 * executed.
 *
 * @param success whether the nanoapp successfully fullfilled a request
 * @param hostEndpoint the host endpoint that sent the request to the nanoapp
 */
void sendResponseMessageToHost(bool success, uint16_t hostEndpoint) {
  auto builder = chre::MakeUnique<FlatBufferBuilder>();
  if (builder.isNull()) {
    LOG_OOM();
  } else {
    chre::power_test::CreateNanoappResponseMessage(*builder, success);

    // CHRE's version of flatbuffers doesn't allow releasing the underlying
    // buffer from the builder so copy it into a new buffer to be sent to the
    // host.
    size_t bufferCopySize = builder->GetSize();
    void *buffer = chreHeapAlloc(bufferCopySize);
    if (buffer == nullptr) {
      LOG_OOM();
    } else {
      memcpy(buffer, builder->GetBufferPointer(), bufferCopySize);
      if (!chreSendMessageToHostEndpoint(
              buffer, bufferCopySize,
              static_cast<uint32_t>(MessageType::NANOAPP_RESPONSE),
              hostEndpoint, chre::heapFreeMessageCallback)) {
        LOGE("Failed to send response message with success %d", success);
      }
    }
  }
}

bool nanoappStart() {
  LOGI("App started on platform ID %" PRIx64, chreGetPlatformId());

  RequestManagerSingleton::init();

  return true;
}

void nanoappHandleEvent(uint32_t senderInstanceId,
                        uint16_t eventType,
                        const void *eventData) {
  // TODO: Print all events corresponding to what the nanoapp can request.
  switch (eventType) {
    case CHRE_EVENT_MESSAGE_FROM_HOST: {
      auto *msg = static_cast<const chreMessageFromHostData *>(eventData);
      bool success = RequestManagerSingleton::get()
          ->handleMessageFromHost(*msg);
      sendResponseMessageToHost(success, msg->hostEndpoint);
      break;
    }
    case CHRE_EVENT_TIMER:
      RequestManagerSingleton::get()->handleTimerEvent(eventData);
      break;
    default:
      LOGD("Received unknown event %" PRIu16, eventType);
  }
}

void nanoappEnd() {
  RequestManagerSingleton::deinit();
  LOGI("Stopped");
}

#ifdef CHRE_NANOAPP_INTERNAL
}  // anonymous namespace
}  // namespace chre

#include "chre/util/nanoapp/app_id.h"
#include "chre/platform/static_nanoapp_init.h"

CHRE_STATIC_NANOAPP_INIT(PowerTest, chre::kPowerTestAppId, 0);
#endif  // CHRE_NANOAPP_INTERNAL
