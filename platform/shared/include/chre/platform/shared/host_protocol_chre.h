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

#ifndef CHRE_PLATFORM_SHARED_HOST_PROTOCOL_CHRE_H_
#define CHRE_PLATFORM_SHARED_HOST_PROTOCOL_CHRE_H_

#include <stdint.h>

#include "chre/platform/shared/host_protocol_common.h"
#include "flatbuffers/flatbuffers.h"

namespace chre {

/**
 * These methods are called from decodeMessageFromHost() and must be implemented
 * by the code that calls it to handle parsed messages.
 */
class HostMessageHandlers {
 public:
  static void handleNanoappMessage(
    uint64_t appId, uint32_t messageType, uint16_t hostEndpoint,
    const void *messageData, size_t messageDataLen);

  static void handleHubInfoRequest();
};

/**
 * A set of helper methods that simplify the encode/decode of FlatBuffers
 * messages used in communications with the host from CHRE.
 */
class HostProtocolChre : public HostProtocolCommon {
 public:
  /**
   * Verifies and decodes a FlatBuffers-encoded CHRE message.
   *
   * @param message Buffer containing message
   * @param messageLen Size of the message, in bytes
   * @param handlers Contains callbacks to process a decoded message
   *
   * @return bool true if the message was successfully decoded, false if it was
   *         corrupted/invalid/unrecognized
   */
  static bool decodeMessageFromHost(const void *message, size_t messageLen);

  /**
   * Refer to the context hub HAL definition for a details of these parameters.
   *
   * @param builder A newly constructed FlatBufferBuilder that will be used to
   *        encode the message
   */
  static void encodeHubInfoResponse(
      flatbuffers::FlatBufferBuilder& builder, const char *name,
      const char *vendor, const char *toolchain, uint32_t legacyPlatformVersion,
      uint32_t legacyToolchainVersion, float peakMips, float stoppedPower,
      float sleepPower, float peakPower, uint32_t maxMessageLen,
      uint64_t platformId, uint32_t version);
};

}  // namespace chre

#endif  // CHRE_PLATFORM_SHARED_HOST_PROTOCOL_CHRE_H_
