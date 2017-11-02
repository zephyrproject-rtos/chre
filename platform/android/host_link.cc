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

#include "chre/platform/android/host_link.h"

#include "chre/core/event_loop_manager.h"
#include "chre/platform/shared/host_protocol_common.h"
#include "chre/util/macros.h"
#include "chre_api/chre/version.h"
#include "chre_host/host_messages_generated.h"

namespace chre {

/**
 * Assigns a vector the contents of a C-style, null-terminated string.
 *
 * @param vector The vector to assign with the contents of a string.
 * @param str The string to assign.
 */
void setVectorToString(std::vector<int8_t> *vector, const char *str) {
  *vector = std::vector<int8_t>(str, str + strlen(str));
}

/**
 * Handles a message directed to a nanoapp from the system.
 *
 * @param message The message to deliver to a nanoapp.
 */
void handleNanoappMessage(const fbs::NanoappMessageT& message) {
  LOGD("handleNanoappMessage");
}

/**
 * Handles a request for information about this context hub instance.
 *
 * @param hostClientId The client ID on the host making the request.
 */
void handleHubInfoRequest(uint16_t hostClientId) {
  LOGD("handleHubInfoRequest");
  fbs::HubInfoResponseT response;
  setVectorToString(&response.name, "CHRE on Android");
  setVectorToString(&response.vendor, "Google");
  setVectorToString(&response.toolchain, "Android NDK API 26 (clang "
    STRINGIFY(__clang_major__) "."
    STRINGIFY(__clang_minor__) "."
    STRINGIFY(__clang_patchlevel__) ")");
  response.platform_version = 0;
  response.toolchain_version = ((__clang_major__ & 0xFF) << 24) |
    ((__clang_minor__ & 0xFF) << 16) |
    (__clang_patchlevel__ & 0xFFFF);
  response.peak_mips = 1000;
  response.stopped_power = 1000;
  response.sleep_power = 1000;
  response.peak_power = 10000;
  response.max_msg_len = CHRE_MESSAGE_TO_HOST_MAX_SIZE;
  response.platform_id = chreGetPlatformId();
  response.chre_platform_version = chreGetVersion();

  fbs::MessageContainerT container;
  container.message.Set(std::move(response));
  container.host_addr.reset(new fbs::HostAddress(hostClientId));

  flatbuffers::FlatBufferBuilder builder;
  auto containerOffset = CreateMessageContainer(builder, &container, nullptr);
  builder.Finish(containerOffset);

  SocketServerSingleton::get()->sendToClientById(
      builder.GetBufferPointer(), builder.GetSize(), hostClientId);
}

/**
 * Handles a request from the host for a list of nanoapps.
 *
 * @param hostClientId The client ID on the host making the request.
 */
void handleNanoappListRequest(uint16_t hostClientId) {
  LOGD("handleNanoappListRequest");
}

/**
 * Handles a request to load a nanoapp.
 *
 * @param hostClientId The client ID on the host making the request.
 * @param loadRequest The details of the nanoapp load request.
 */
void handleLoadNanoappRequest(
    uint16_t hostClientId, const fbs::LoadNanoappRequestT& loadRequest) {
  LOGD("handleLoadNanoappRequest");
}

/**
 * Handles a request to unload a nanoapp.
 *
 * @param hostClientId The client ID on the host making the request.
 * @param unloadRequest The details of the nanoapp unload request.
 */
void handleUnloadNanoappRequest(
    uint16_t hostClientId, const fbs::UnloadNanoappRequestT& unloadRequest) {
  LOGD("handleUnloadNanoappRequest");
}

/**
 * Handles a request for a debug dump.
 *
 * @param hostClientId The client OD on the host making the request.
 */
void handleDebugDumpRequest(uint16_t hostClientId) {
  LOGD("handleDebugDumpRequest");
}

bool handleMessageFromHost(void *message, size_t length) {
  bool success = HostProtocolCommon::verifyMessage(message, length);
  if (success) {
    fbs::MessageContainerT container;
    fbs::GetMessageContainer(message)->UnPackTo(&container);
    uint16_t hostClientId = container.host_addr->client_id();
    switch (container.message.type) {
      case fbs::ChreMessage::NanoappMessage:
        handleNanoappMessage(*container.message.AsNanoappMessage());
        break;

      case fbs::ChreMessage::HubInfoRequest:
        handleHubInfoRequest(hostClientId);
        break;

      case fbs::ChreMessage::NanoappListRequest:
        handleNanoappListRequest(hostClientId);
        break;

      case fbs::ChreMessage::LoadNanoappRequest:
        handleLoadNanoappRequest(hostClientId,
                                 *container.message.AsLoadNanoappRequest());
        break;

      case fbs::ChreMessage::UnloadNanoappRequest:
        handleUnloadNanoappRequest(hostClientId,
                                   *container.message.AsUnloadNanoappRequest());
        break;

      case fbs::ChreMessage::DebugDumpRequest:
        handleDebugDumpRequest(hostClientId);
        break;

      default:
        LOGW("Got invalid/unexpected message type %" PRIu8,
             static_cast<uint8_t>(container.message.type));
        success = false;
    }
  }

  return success;
}

}  // namespace chre
