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

#include <sys/socket.h>
#include <sys/types.h>

#include <cutils/sockets.h>

/**
 * @file
 * A test utility that connects to the CHRE daemon that runs on the apps
 * processor of MSM chipsets, which is used to help test basic functionality.
 */

int main() {
  int fd = socket_local_client("chre", ANDROID_SOCKET_NAMESPACE_RESERVED,
                               SOCK_SEQPACKET);
  if (fd < 0) {
    LOGE("Couldn't connect to socket");
    return -1;
  }

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
  ssize_t bytesSent = send(fd, sendBuf, sizeof(sendBuf), 0);
  if (bytesSent != sizeof(sendBuf)) {
    LOGE("Couldn't send message to socket (returned %zd): %s", bytesSent,
         strerror(errno));
  } else {
    LOGI("Waiting on response...");
    uint8_t recvBuf[4096];
    memset(recvBuf, 0, sizeof(recvBuf));
    ssize_t bytesReceived = recv(fd, recvBuf, sizeof(recvBuf), 0);
    if (bytesReceived < 0) {
      LOGE("Couldn't receive data: %s", strerror(errno));
    } else if (bytesReceived == 0) {
      LOGE("Remote end hung up before we could get response");
    } else {
      LOGI("Got response with %zd bytes: %02x %02x %02x...", bytesReceived,
           recvBuf[0], recvBuf[1], recvBuf[2]);
    }
  }

  close(fd);
  return 0;
}
