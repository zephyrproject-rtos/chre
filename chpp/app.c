/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "chpp/app.h"

void chppAppInit(struct ChppAppState *appContext,
                 struct ChppTransportState *transportContext) {
  CHPP_NOT_NULL(appContext);
  CHPP_NOT_NULL(transportContext);

  memset(appContext, 0, sizeof(struct ChppAppState));

  appContext->transportContext = transportContext;
}

void chppProcessRxDatagram(struct ChppAppState *context, uint8_t *buf,
                           size_t len) {
  // TODO

  // Loopback functionality
  if ((len >= 2) && (buf[0] == CHPP_DATAGRAM_HANDLE_LOOPBACK) &&
      (buf[1] == CHPP_DATAGRAM_TYPE_CLIENT_REQUEST)) {
    // Datagram is a loopback datagram

    uint8_t *response = chppMalloc(len);
    memcpy(response, buf, len);

    // We are done with buf
    chppAppProcessDoneCb(context->transportContext, buf);

    // Modify response per loopback spec.
    response[1] = CHPP_DATAGRAM_TYPE_SERVER_RESPONSE;

    // Send out response datagram
    if (!chppEnqueueTxDatagram(context->transportContext, response, len)) {
      LOGE("Tx Queue full. Dropping loopback datagram of %zu bytes", len);
      chppFree(response);
    }
  }
}
