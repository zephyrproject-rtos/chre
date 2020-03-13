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
bool chppDatagramLenIsOk(struct ChppAppState *context, uint8_t handle,
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
      chppDispatchNonHandle(context, buf, len);
      break;

    case CHPP_HANDLE_LOOPBACK:
      chppDispatchLoopback(context, buf, len);
      break;

    case CHPP_HANDLE_DISCOVERY:
      chppDispatchDiscovery(context, buf, len);
      break;

    default:
      LOGE("Invalid predefined service handle %d", rxHeader->handle);
  }
}

/*
 * Processes an Rx Datagram from the transport layer that is determined to be
 * for a negotiated CHPP service and with a correct minimum length.
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

bool chppDatagramLenIsOk(struct ChppAppState *context, uint8_t handle,
                         size_t len) {
  size_t minLen = SIZE_MAX;

  if (handle < CHPP_HANDLE_NEGOTIATED_RANGE_START) {
    // Predefined services

    switch (handle) {
      case CHPP_HANDLE_NONE:
        minLen = sizeof_member(struct ChppAppHeader, handle);
        break;

      case CHPP_HANDLE_LOOPBACK:
        minLen = sizeof_member(struct ChppAppHeader, handle) +
                 sizeof_member(struct ChppAppHeader, type);
        break;

      case CHPP_HANDLE_DISCOVERY:
        minLen = sizeof(struct ChppAppHeader);
        break;

      default:
        LOGE("Invalid predefined service handle %d", handle);
    }
  } else {
    // TODO for negotiated services based on their chppServiceObject
    UNUSED_VAR(context);
  }

  if (len < minLen) {
    LOGE("Received datagram too short for handle=%d, len=%u", handle, len);
  }
  return (len >= minLen);
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
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;

  if (chppDatagramLenIsOk(context, rxHeader->handle, len)) {
    if (rxHeader->handle < CHPP_HANDLE_NEGOTIATED_RANGE_START) {
      // Predefined Services
      chppProcessPredefinedService(context, buf, len);
    } else {
      // Negotiated Services
      chppProcessNegotiatedService(context, buf, len);
    }
  }

  chppAppProcessDoneCb(context->transportContext, buf);
}
