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

#include "chpp/services/loopback.h"

/************************************************
 *  Prototypes
 ***********************************************/

/************************************************
 *  Private Functions
 ***********************************************/

/************************************************
 *  Public Functions
 ***********************************************/

void chppDispatchLoopback(struct ChppAppState *context, uint8_t *buf,
                          size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;

  if (rxHeader->type == CHPP_MESSAGE_TYPE_CLIENT_REQUEST) {
    // We need to respond to a loopback datagram

    uint8_t *response = chppMalloc(len);
    if (response == NULL) {
      LOGE("OOM crafting loopback message len = %zu", len);

      // TODO: we should reply back to the client with an error

    } else {
      // Copy received datagram
      memcpy(response, buf, len);

      // Modify response message type per loopback spec.
      struct ChppAppHeader *responseHeader = (struct ChppAppHeader *)response;
      responseHeader->type = CHPP_MESSAGE_TYPE_SERVER_RESPONSE;

      // Send out response datagram
      chppEnqueueTxDatagramOrFail(context->transportContext, response, len);
    }
  } else {
    LOGE("Unknown Loopback message type = %d", rxHeader->type);
  }
}
