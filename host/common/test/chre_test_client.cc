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
using android::chre::IChreMessageHandlers;
using android::chre::SocketClient;
using android::chre::HostProtocolHost;
using flatbuffers::FlatBufferBuilder;

namespace {

//! The host endpoint we use when sending; set to CHRE_HOST_ENDPOINT_UNSPECIFIED
constexpr uint16_t kHostEndpoint = 0xfffe;

class SocketCallbacks : public SocketClient::ICallbacks,
                        public IChreMessageHandlers {
 public:
  void onMessageReceived(const void *data, size_t length) override {
    if (!HostProtocolHost::decodeMessageFromChre(data, length, *this)) {
      LOGE("Failed to decode message");
    }
  }

  void onConnected() override {
    LOGI("Socket (re)connected");
  }

  void onConnectionAborted() override {
    LOGI("Socket (re)connection aborted");
  }

  void onDisconnected() override {
    LOGI("Socket disconnected");
  }

  void handleNanoappMessage(
      uint64_t appId, uint32_t messageType, uint16_t hostEndpoint,
      const void * /*messageData*/, size_t messageLen) override {
    LOGI("Got message from nanoapp 0x%" PRIx64 " to endpoint 0x%" PRIx16
         " with type 0x%" PRIx32 " and length %zu", appId, hostEndpoint,
         messageType, messageLen);
  }

  void handleHubInfoResponse(
      const char *name, const char *vendor,
      const char *toolchain, uint32_t legacyPlatformVersion,
      uint32_t legacyToolchainVersion, float peakMips, float stoppedPower,
      float sleepPower, float peakPower, uint32_t maxMessageLen,
      uint64_t platformId, uint32_t version) override {
    LOGI("Got hub info response:");
    LOGI("  Name: '%s'", name);
    LOGI("  Vendor: '%s'", vendor);
    LOGI("  Toolchain: '%s'", toolchain);
    LOGI("  Legacy versions: platform 0x%08" PRIx32 " toolchain 0x%08" PRIx32,
         legacyPlatformVersion, legacyToolchainVersion);
    LOGI("  MIPS %.2f Power (mW): stopped %.2f sleep %.2f peak %.2f",
         peakMips, stoppedPower, sleepPower, peakPower);
    LOGI("  Max message len: %" PRIu32, maxMessageLen);
    LOGI("  Platform ID: 0x%016" PRIx64 " Version: 0x%08x" PRIx32,
         platformId, version);
  }

  void handleNanoappListResponse(
      const flatbuffers::Vector<flatbuffers::Offset<
            ::chre::fbs::NanoappListEntry>>& nanoapps) {
    LOGI("Got nanoapp list response with %" PRIu32 " apps:", nanoapps.size());
    for (const auto *nanoapp : nanoapps){
      LOGI("  App ID 0x%016" PRIx64 " version 0x%" PRIx32 " enabled %d system "
           "%d", nanoapp->app_id(), nanoapp->version(), nanoapp->enabled(),
           nanoapp->is_system());
    }
  }
};

void requestHubInfo(SocketClient& client) {
  FlatBufferBuilder builder(64);
  HostProtocolHost::encodeHubInfoRequest(builder);

  LOGI("Sending hub info request (%u bytes)", builder.GetSize());
  if (!client.sendMessage(builder.GetBufferPointer(), builder.GetSize())) {
    LOGE("Failed to send message");
  }
}

void requestNanoappList(SocketClient& client) {
  FlatBufferBuilder builder(64);
  HostProtocolHost::encodeNanoappListRequest(builder);

  LOGI("Sending app list request (%u bytes)", builder.GetSize());
  if (!client.sendMessage(builder.GetBufferPointer(), builder.GetSize())) {
    LOGE("Failed to send message");
  }
}

void sendMessageToNanoapp(SocketClient& client) {
  FlatBufferBuilder builder(64);
  uint8_t messageData[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
  HostProtocolHost::encodeNanoappMessage(
      builder, 0, kHostEndpoint, 1234, messageData, sizeof(messageData));

  LOGI("Sending message to nanoapp (%u bytes w/%zu bytes of payload)",
       builder.GetSize(), sizeof(messageData));
  if (!client.sendMessage(builder.GetBufferPointer(), builder.GetSize())) {
    LOGE("Failed to send message");
  }
}

}

int main() {
  int ret = -1;
  SocketClient client;
  sp<SocketCallbacks> callbacks = new SocketCallbacks();

  if (!client.connect("chre", callbacks)) {
    LOGE("Couldn't connect to socket");
  } else {
    requestHubInfo(client);
    requestNanoappList(client);
    sendMessageToNanoapp(client);

    LOGI("Sleeping, waiting on responses");
    std::this_thread::sleep_for(std::chrono::seconds(5));
  }

 return 0;
}
