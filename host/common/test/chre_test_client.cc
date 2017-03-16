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
#include "chre_host/log.h"
#include "chre_host/socket_client.h"

#include <inttypes.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <thread>

#include <cutils/sockets.h>
#include <utils/StrongPointer.h>

/**
 * @file
 * A test utility that connects to the CHRE daemon that runs on the apps
 * processor of MSM chipsets, which is used to help test basic functionality.
 */

using android::sp;
using android::chre::SocketClient;
using chre::HostProtocol;
using flatbuffers::FlatBufferBuilder;

namespace {

//! The host endpoint we use when sending; set to CHRE_HOST_ENDPOINT_UNSPECIFIED
constexpr uint16_t kHostEndpoint = 0xfffe;

class SocketCallbacks : public SocketClient::ICallbacks,
                        public HostProtocol::IMessageHandlers {
 public:
  void onMessageReceived(const void *data, size_t length) override {
    if (!HostProtocol::decodeMessage(data, length, *this)) {
      LOGE("Failed to decode message");
    }
  }

  void onSocketDisconnectedByRemote() override {
    LOGI("Socket disconnected");
  }

  void handleNanoappMessage(
      uint64_t appId, uint32_t messageType, uint16_t hostEndpoint,
      const void * /*messageData*/, size_t messageLen) override {
    LOGI("Got message from nanoapp 0x%" PRIx64 " to endpoint 0x%" PRIx16
         " with type 0x%" PRIx32 " and length %zu", appId, hostEndpoint,
         messageType, messageLen);
  }
};

}

int main() {
  int ret = -1;
  SocketClient client;
  sp<SocketCallbacks> callbacks = new SocketCallbacks();

  if (!client.connect("chre", true /*reconnectAutomatically*/, callbacks)) {
    LOGE("Couldn't connect to socket");
  } else {
    FlatBufferBuilder builder(2048);
    uint8_t messageData[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    HostProtocol::encodeNanoappMessage(
        builder, 0, kHostEndpoint, 1234, messageData, sizeof(messageData));

    LOGI("Sending message (%u bytes)", builder.GetSize());
    if (!client.sendMessage(builder.GetBufferPointer(), builder.GetSize())) {
      LOGE("Failed to send message");
    }

    LOGI("Sleeping, waiting on responses");
    std::this_thread::sleep_for(std::chrono::seconds(5));
  }

 return 0;
}
