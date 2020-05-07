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
#include "chpp/pal_api.h"
#include "chpp/services.h"

#include "chpp/clients/discovery.h"

#include "chpp/services/discovery.h"
#include "chpp/services/loopback.h"
#include "chpp/services/nonhandle.h"

/************************************************
 *  Prototypes
 ***********************************************/

void chppProcessPredefinedService(struct ChppAppState *context, uint8_t *buf,
                                  size_t len);
void chppProcessNegotiatedService(struct ChppAppState *context, uint8_t *buf,
                                  size_t len);
void chppProcessPredefinedClient(struct ChppAppState *context, uint8_t *buf,
                                 size_t len);
void chppProcessNegotiatedClient(struct ChppAppState *context, uint8_t *buf,
                                 size_t len);
bool chppDatagramLenIsOk(struct ChppAppState *context, uint8_t handle,
                         size_t len);
ChppDispatchFunction *chppDispatchFunctionOfService(
    struct ChppAppState *context, uint8_t handle);
ChppDispatchFunction *chppDispatchFunctionOfClient(struct ChppAppState *context,
                                                   uint8_t handle);
inline struct ChppService *chppServiceOfHandle(struct ChppAppState *appContext,
                                               uint8_t handle);

/************************************************
 *  Private Functions
 ***********************************************/

/**
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
    case CHPP_HANDLE_LOOPBACK:
      chppDispatchLoopbackService(context, buf, len);
      break;

    case CHPP_HANDLE_DISCOVERY:
      chppDispatchDiscoveryService(context, buf, len);
      break;

    default:
      LOGE("Invalid predefined service handle %" PRIu8, rxHeader->handle);
  }
}

/**
 * Processes an Rx Datagram from the transport layer that is determined to be
 * for a negotiated CHPP service and with a correct minimum length.
 *
 * @param context Maintains status for each app layer instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
void chppProcessNegotiatedService(struct ChppAppState *context, uint8_t *buf,
                                  size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;

  // Already validated that handle is OK
  ChppDispatchFunction *dispatchFunc =
      chppDispatchFunctionOfService(context, rxHeader->handle);
  dispatchFunc(context, buf, len);
}

/**
 * Processes an Rx Datagram from the transport layer that is determined to be
 * for a predefined CHPP client.
 *
 * @param context Maintains status for each app layer instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
void chppProcessPredefinedClient(struct ChppAppState *context, uint8_t *buf,
                                 size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;

  switch (rxHeader->handle) {
    case CHPP_HANDLE_LOOPBACK:
      // TODO
      break;

    case CHPP_HANDLE_DISCOVERY:
      chppDispatchDiscoveryClient(context, buf, len);
      break;

    default:
      LOGE("Invalid predefined service handle %" PRIu8, rxHeader->handle);
  }
}

/**
 * Processes an Rx Datagram from the transport layer that is determined to be
 * for a negotiated CHPP client and with a correct minimum length.
 *
 * @param context Maintains status for each app layer instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
void chppProcessNegotiatedClient(struct ChppAppState *context, uint8_t *buf,
                                 size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;

  // Already validated that handle is OK
  ChppDispatchFunction *dispatchFunc =
      chppDispatchFunctionOfClient(context, rxHeader->handle);
  dispatchFunc(context, buf, len);
}

/**
 * Verifies if the length of a Rx Datagram from the transport layer is
 * sufficient for the associated service.
 *
 * @param context Maintains status for each app layer instance.
 * @param handle Handle number for the service.
 * @param len Length of the datagram in bytes.
 *
 * @return true if length is ok.
 */
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
        LOGE("Invalid predefined handle %" PRIu8, handle);
    }

  } else {
    // Negotiated services

    minLen = chppServiceOfHandle(context, handle)->minLength;
  }

  if (len < minLen) {
    LOGE("Received datagram too short for handle=%" PRIu8 ", len=%zu", handle,
         len);
  }
  return (len >= minLen);
}

/**
 * Returns the service dispatch function of a particular negotiated handle.
 *
 * @param context Maintains status for each app layer instance.
 * @param handle Handle number for the service.
 *
 * @return Pointer to a function that dispatches incoming datagrams for any
 * particular service.
 */
ChppDispatchFunction *chppDispatchFunctionOfService(
    struct ChppAppState *context, uint8_t handle) {
  return (
      context->registeredServices[handle - CHPP_HANDLE_NEGOTIATED_RANGE_START]
          ->dispatchFunctionPtr);
}

/**
 * Returns the client dispatch function of a particular negotiated handle.
 *
 * @param context Maintains status for each app layer instance.
 * @param handle Handle number for the service.
 *
 * @return Pointer to a function that dispatches incoming datagrams for any
 * particular service.
 */
ChppDispatchFunction *chppDispatchFunctionOfClient(struct ChppAppState *context,
                                                   uint8_t handle) {
  // TODO

  UNUSED_VAR(context);
  UNUSED_VAR(handle);
  return (NULL);
}

/**
 * Returns a pointer to the ChppService struct of a particular negotiated
 * service handle.
 *
 * @param context Maintains status for each app layer instance.
 * @param handle Handle number for the service.
 *
 * @return Pointer to the ChppService struct of a particular service handle.
 */
inline struct ChppService *chppServiceOfHandle(struct ChppAppState *appContext,
                                               uint8_t handle) {
  return (appContext
              ->registeredServiceContexts[handle -
                                          CHPP_HANDLE_NEGOTIATED_RANGE_START]);
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

  chppPalSystemApiInit(appContext);

  chppRegisterCommonServices(appContext);
}

void chppAppDeinit(struct ChppAppState *appContext) {
  // TODO

  chppPalSystemApiDeinit(appContext);
}

void chppProcessRxDatagram(struct ChppAppState *context, uint8_t *buf,
                           size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;

  if (chppDatagramLenIsOk(context, rxHeader->handle, len)) {
    if (rxHeader->handle >=
        context->registeredServiceCount + CHPP_HANDLE_NEGOTIATED_RANGE_START) {
      LOGE("Received datagram for invalid handle: %" PRIu8
           ", len = %zu, type = %#x, transaction ID = %" PRIu8,
           rxHeader->handle, len, rxHeader->type, rxHeader->transaction);

    } else if (rxHeader->handle == CHPP_HANDLE_NONE) {
      chppDispatchNonHandle(context, buf, len);

    } else {
      switch (rxHeader->type) {
        case CHPP_MESSAGE_TYPE_CLIENT_REQUEST:
        case CHPP_MESSAGE_TYPE_CLIENT_NOTIFICATION: {
          if (rxHeader->handle < CHPP_HANDLE_NEGOTIATED_RANGE_START) {
            chppProcessPredefinedService(context, buf, len);
          } else {
            chppProcessNegotiatedService(context, buf, len);
          }
          break;
        }

        case CHPP_MESSAGE_TYPE_SERVER_RESPONSE:
        case CHPP_MESSAGE_TYPE_SERVER_NOTIFICATION: {
          if (rxHeader->handle < CHPP_HANDLE_NEGOTIATED_RANGE_START) {
            chppProcessPredefinedClient(context, buf, len);
          } else {
            chppProcessNegotiatedClient(context, buf, len);
          }
          break;
        }

        default: {
          LOGE(
              "Received unknown message type: %#x, len = %zu, transaction ID = "
              "%" PRIu8,
              rxHeader->type, len, rxHeader->transaction);
          chppEnqueueTxErrorDatagram(context->transportContext,
                                     CHPP_TRANSPORT_ERROR_APPLAYER);
        }
      }
    }
  }

  chppAppProcessDoneCb(context->transportContext, buf);
}
