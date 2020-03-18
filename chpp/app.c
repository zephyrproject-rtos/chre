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

/************************************************
 *  Prototypes
 ***********************************************/

void chppProcessPredefinedService(struct ChppAppState *context, uint8_t *buf,
                                  size_t len);
void chppProcessNegotiatedService(struct ChppAppState *context, uint8_t *buf,
                                  size_t len);
void chppProcessNonHandle(struct ChppAppState *context, uint8_t *buf,
                          size_t len);
void chppProcessLoopback(struct ChppAppState *context, uint8_t *buf,
                         size_t len);
void chppProcessDiscovery(struct ChppAppState *context, uint8_t *buf,
                          size_t len);

/************************************************
 *  Private Functions
 ***********************************************/

/*
 * Processes an Rx Datagram from the transport layer that is determined to be
 * for a predefined CHPP service.
 *
 * @param context Maintains status for each app layer instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
void chppProcessPredefinedService(struct ChppAppState *context, uint8_t *buf,
                                  size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;

  switch (rxHeader->handle) {
    case CHPP_HANDLE_NONE:
      chppProcessNonHandle(context, buf, len);
      break;

    case CHPP_HANDLE_LOOPBACK:
      chppProcessLoopback(context, buf, len);
      break;

    case CHPP_HANDLE_DISCOVERY:
      chppProcessDiscovery(context, buf, len);
      break;

    default:
      LOGE("Invalid predefined service handle %d", rxHeader->handle);
  }
}

/*
 * Processes an Rx Datagram from the transport layer that is determined to be
 * for a negotiated CHPP service.
 *
 * @param context Maintains status for each app layer instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
void chppProcessNegotiatedService(struct ChppAppState *context, uint8_t *buf,
                                  size_t len) {
  // TODO

  UNUSED_VAR(context);
  UNUSED_VAR(buf);
  UNUSED_VAR(len);
}

/*
 * Processes an Rx Datagram from the transport layer for systems that are using
 * the optional handleless CHPP communication service. Does not need to be
 * implemented on systems that are not using handleless communication.
 *
 * @param context Maintains status for each app layer instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
void chppProcessNonHandle(struct ChppAppState *context, uint8_t *buf,
                          size_t len) {
  UNUSED_VAR(context);
  UNUSED_VAR(buf);
  UNUSED_VAR(len);
}

/*
 * Processes an Rx Datagram from the transport layer that is determined to be
 * for the CHPP Loopback Service.
 *
 * @param context Maintains status for each app layer instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
void chppProcessLoopback(struct ChppAppState *context, uint8_t *buf,
                         size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;

  // Loopback functionality
  if ((len >= sizeof(rxHeader->handle) + sizeof(rxHeader->type)) &&
      (rxHeader->type == CHPP_MESSAGE_TYPE_CLIENT_REQUEST)) {
    // We need to respond to a loopback datagram

    // Copy received datagram
    uint8_t *response = chppMalloc(len);
    memcpy(response, buf, len);

    // We are done with buf
    chppAppProcessDoneCb(context->transportContext, buf);

    // Modify response message type per loopback spec.
    struct ChppAppHeader *responseHeader = (struct ChppAppHeader *)response;
    responseHeader->type = CHPP_MESSAGE_TYPE_SERVER_RESPONSE;

    // Send out response datagram
    if (!chppEnqueueTxDatagram(context->transportContext, response, len)) {
      LOGE("Tx Queue full. Dropping loopback datagram of %zu bytes", len);
      chppFree(response);
    }
  }
}

/*
 * Processes an Rx Datagram from the transport layer that is determined to be
 * for the CHPP Discovery Service.
 *
 * @param context Maintains status for each app layer instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
void chppProcessDiscovery(struct ChppAppState *context, uint8_t *buf,
                          size_t len) {
  // TODO

  UNUSED_VAR(context);
  UNUSED_VAR(buf);
  UNUSED_VAR(len);
}

/************************************************
 *  Public Functions
 ***********************************************/

void chppAppInit(struct ChppAppState *appContext,
                 struct ChppTransportState *transportContext) {
  CHPP_NOT_NULL(appContext);
  CHPP_NOT_NULL(transportContext);

  memset(appContext, 0, sizeof(struct ChppAppState));

  appContext->transportContext = transportContext;
}

void chppProcessRxDatagram(struct ChppAppState *context, uint8_t *buf,
                           size_t len) {
  CHPP_ASSERT(len > 0);
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;

  if (rxHeader->handle < CHPP_HANDLE_NEGOTIATED_RANGE_START) {
    // Predefined Services
    chppProcessPredefinedService(context, buf, len);
  } else {
    // Negotiated Services
    chppProcessNegotiatedService(context, buf, len);
  }
}
