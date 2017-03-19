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

#include "chre_host/host_protocol_host.h"

#include <inttypes.h>
#include <string.h>

#include "chre/platform/shared/host_messages_generated.h"
#include "chre_host/log.h"

using flatbuffers::FlatBufferBuilder;
using flatbuffers::Offset;

// Aliased for consistency with the way these symbols are referenced in
// CHRE-side code
namespace fbs = ::chre::fbs;

namespace android {
namespace chre {

namespace {

/**
 * Checks that a string encapsulated as a byte vector is null-terminated, and
 * if it is, returns a pointer to the vector's data. Otherwise returns null.
 *
 * @param vec Target vector, can be null
 *
 * @return Pointer to the vector's data, or null
 */
const char *getStringFromByteVector(const flatbuffers::Vector<int8_t> *vec) {
  constexpr int8_t kNullChar = 0;
  const char *str = nullptr;

  // Check that the vector is present, non-empty, and null-terminated
  if (vec != nullptr && vec->size() > 0
      && (*vec)[vec->size() - 1] == kNullChar) {
    str = reinterpret_cast<const char *>(vec->data());
  }

  return str;
}

}  // anonymous namespace

bool HostProtocolHost::decodeMessageFromChre(
    const void *message, size_t messageLen, IChreMessageHandlers& handlers) {
  bool success = verifyMessage(message, messageLen);
  if (success) {
    const fbs::MessageContainer *container = fbs::GetMessageContainer(message);

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

      case fbs::ChreMessage::HubInfoResponse: {
        const auto *resp = static_cast<const fbs::HubInfoResponse *>(
            container->message());

        const char *name = getStringFromByteVector(resp->name());
        const char *vendor = getStringFromByteVector(resp->vendor());
        const char *toolchain = getStringFromByteVector(resp->toolchain());

        handlers.handleHubInfoResponse(
            name, vendor, toolchain, resp->platform_version(),
            resp->toolchain_version(), resp->peak_mips(), resp->stopped_power(),
            resp->sleep_power(), resp->peak_power(), resp->max_msg_len(),
            resp->platform_id(), resp->chre_platform_version());
        break;
      }

      default:
        LOGW("Got invalid/unexpected message type %" PRIu8,
             static_cast<uint8_t>(container->message_type()));
        success = false;
    }
  }

  return success;
}

void HostProtocolHost::encodeHubInfoRequest(FlatBufferBuilder& builder) {
  auto request = fbs::CreateHubInfoRequest(builder);
  auto container = fbs::CreateMessageContainer(
      builder, fbs::ChreMessage::HubInfoRequest, request.Union());
  builder.Finish(container);
}

}  // namespace chre
}  // namespace android
