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

#include "chre_host/log.h"
#include "chre_host/socket_client.h"

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

namespace {

class SocketCallbacks : public SocketClient::ICallbacks {
 public:
  void onMessageReceived(const void *data, size_t length) override {
    LOGD("Got message @ %p with %zu bytes", data, length);
  }

  void onSocketDisconnectedByRemote() override {
    LOGI("Socket disconnected");
  }
};

}

int main() {
  int ret = -1;
  SocketClient client;
  sp<SocketCallbacks> callbacks = new SocketCallbacks();

  if (!client.connect("chre", false /*reconnectAutomatically*/, callbacks)) {
    LOGE("Couldn't connect to socket");
  } else {
    // TODO: this should use the client library - right now using a hard-coded
    // message to nanoapp for testing in conjunction with MessageWorld
    const uint8_t sendBuf[] = {
      0x0c, 0x00, 0x00, 0x00, 0x08, 0x00, 0x0c, 0x00,
      0x07, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x01, 0x10, 0x00, 0x00, 0x00,
      0x0c, 0x00, 0x10, 0x00, 0x00, 0x00, 0x08, 0x00,
      0x06, 0x00, 0x0c, 0x00, 0x0c, 0x00, 0x00, 0x00,
      0x00, 0x00, 0xfe, 0xff, 0xd2, 0x04, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    };
    LOGI("Sending message (%zu bytes)", sizeof(sendBuf));
    if (!client.sendMessage(sendBuf, sizeof(sendBuf))) {
      LOGE("Failed to send message");
    }

    LOGI("Sleeping, waiting on responses");
    std::this_thread::sleep_for(std::chrono::seconds(5));

    LOGI("Performing graceful disconnect");
    client.disconnect();

    LOGI("Disconnect again");
    client.disconnect();
  }

 return 0;
}
