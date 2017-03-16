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

#ifndef CHRE_PLATFORM_SHARED_HOST_PROTOCOL_H_
#define CHRE_PLATFORM_SHARED_HOST_PROTOCOL_H_

/**
 * @file
 * A set of helper methods that simplify the encode/decode of FlatBuffers
 * messages used in communication with CHRE. This file can be used both within
 * CHRE and on the host-side.
 */

#include <stdint.h>

#include "flatbuffers/flatbuffers.h"

namespace chre {

class HostProtocol {
 public:
  class IMessageHandlers {
   public:
    virtual ~IMessageHandlers() = default;

    virtual void handleNanoappMessage(
        uint64_t appId, uint32_t messageType, uint16_t hostEndpoint,
        const void *messageData, size_t messageDataLen) = 0;
  };

  /**
   * Encodes a message to/from a nanoapp using the given FlatBufferBuilder and
   * supplied parameters.
   *
   * @param builder A newly constructed FlatBufferBuilder that will be used to
   *        encode the message. It will be finalized before returning from this
   *        function.
   */
  static void encodeNanoappMessage(
      flatbuffers::FlatBufferBuilder& builder, uint64_t appId,
      uint32_t messageType, uint16_t hostEndpoint, const void *messageData,
      size_t messageDataLen);

  /**
   * Verifies and decodes a FlatBuffers-encoded CHRE message.
   *
   * @param message Buffer containing message
   * @param messageLen Size of the message, in bytes
   * @param handlers Contains callbacks to process a decoded message
   *
   * @return true if the message was successfully decoded, false if it was
   *         corrupted/invalid/unrecognized
   */
  static bool decodeMessage(const void *message, size_t messageLen,
                            IMessageHandlers& handlers);

};

}  // namespace chre

#endif  // CHRE_PLATFORM_SHARED_HOST_PROTOCOL_H_
