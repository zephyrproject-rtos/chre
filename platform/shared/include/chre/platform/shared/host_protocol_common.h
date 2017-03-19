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

#ifndef CHRE_PLATFORM_SHARED_HOST_PROTOCOL_COMMON_H_
#define CHRE_PLATFORM_SHARED_HOST_PROTOCOL_COMMON_H_

#include <stdint.h>

#include "flatbuffers/flatbuffers.h"

namespace chre {

/**
 * Functions that are shared between the CHRE and host to assist with
 * communications between the two. Note that normally these functions are
 * accessed through a derived class like chre::HostProtocolChre (CHRE-side) or
 * android::chre:HostProtocolHost (host-side).
 */
class HostProtocolCommon {
 public:
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

 protected:
   static flatbuffers::Offset<flatbuffers::Vector<int8_t>>
       addStringAsByteVector(flatbuffers::FlatBufferBuilder& builder,
                             const char *str);
   static bool verifyMessage(const void *message, size_t messageLen);
};

}  // namespace chre

#endif  // CHRE_PLATFORM_SHARED_HOST_PROTOCOL_COMMON_H_
