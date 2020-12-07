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

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "chpp/clients.h"
#include "chpp/clients/discovery.h"
#ifdef CHPP_CLIENT_ENABLED_LOOPBACK
#include "chpp/clients/loopback.h"
#endif
#ifdef CHPP_CLIENT_ENABLED_TIMESYNC
#include "chpp/clients/timesync.h"
#endif
#include "chpp/log.h"
#include "chpp/macros.h"
#include "chpp/notifier.h"
#include "chpp/pal_api.h"
#include "chpp/services.h"
#include "chpp/services/discovery.h"
#include "chpp/services/loopback.h"
#include "chpp/services/nonhandle.h"
#include "chpp/services/timesync.h"

/************************************************
 *  Prototypes
 ***********************************************/

static bool chppProcessPredefinedClientRequest(struct ChppAppState *context,
                                               uint8_t *buf, size_t len);
static bool chppProcessPredefinedServiceResponse(struct ChppAppState *context,
                                                 uint8_t *buf, size_t len);
static bool chppProcessPredefinedClientNotification(
    struct ChppAppState *context, uint8_t *buf, size_t len);
static bool chppProcessPredefinedServiceNotification(
    struct ChppAppState *context, uint8_t *buf, size_t len);

static bool chppDatagramLenIsOk(struct ChppAppState *context,
                                struct ChppAppHeader *rxHeader, size_t len);
ChppDispatchFunction *chppGetDispatchFunction(struct ChppAppState *context,
                                              uint8_t handle,
                                              enum ChppMessageType type);
ChppResetNotifierFunction *chppGetClientResetNotifierFunction(
    struct ChppAppState *context, uint8_t index);
ChppResetNotifierFunction *chppGetServiceResetNotifierFunction(
    struct ChppAppState *context, uint8_t index);
static inline const struct ChppService *chppServiceOfHandle(
    struct ChppAppState *appContext, uint8_t handle);
static inline const struct ChppClient *chppClientOfHandle(
    struct ChppAppState *appContext, uint8_t handle);
static inline void *chppServiceContextOfHandle(struct ChppAppState *appContext,
                                               uint8_t handle);
static inline void *chppClientContextOfHandle(struct ChppAppState *appContext,
                                              uint8_t handle);
static void *chppClientServiceContextOfHandle(struct ChppAppState *appContext,
                                              uint8_t handle,
                                              enum ChppMessageType type);

static void chppProcessPredefinedHandleDatagram(struct ChppAppState *context,
                                                uint8_t *buf, size_t len);
static void chppProcessNegotiatedHandleDatagram(struct ChppAppState *context,
                                                uint8_t *buf, size_t len);

/************************************************
 *  Private Functions
 ***********************************************/

/**
 * Processes a client request that is determined to be for a predefined CHPP
 * service.
 *
 * @param context Maintains status for each app layer instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 *
 * @return False if handle is invalid. True otherwise.
 */
static bool chppProcessPredefinedClientRequest(struct ChppAppState *context,
                                               uint8_t *buf, size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;
  bool handleValid = true;
  bool dispatchResult = true;

  switch (rxHeader->handle) {
    case CHPP_HANDLE_LOOPBACK: {
      dispatchResult = chppDispatchLoopbackClientRequest(context, buf, len);
      break;
    }

    case CHPP_HANDLE_TIMESYNC: {
      dispatchResult = chppDispatchTimesyncClientRequest(context, buf, len);
      break;
    }

    case CHPP_HANDLE_DISCOVERY: {
      dispatchResult = chppDispatchDiscoveryClientRequest(context, buf, len);
      break;
    }

    default: {
      handleValid = false;
    }
  }

  if (dispatchResult == false) {
    CHPP_LOGE("Handle=%" PRIu8
              " received unknown client request. command=%#x, transaction ID="
              "%" PRIu8,
              rxHeader->handle, rxHeader->command, rxHeader->transaction);
  }

  return handleValid;
}

/**
 * Processes a service response that is determined to be for a predefined CHPP
 * client.
 *
 * @param context Maintains status for each app layer instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 *
 * @return False if handle is invalid. True otherwise.
 */
static bool chppProcessPredefinedServiceResponse(struct ChppAppState *context,
                                                 uint8_t *buf, size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;
  bool handleValid = true;
  bool dispatchResult = true;

  switch (rxHeader->handle) {
#ifdef CHPP_CLIENT_ENABLED_LOOPBACK
    case CHPP_HANDLE_LOOPBACK: {
      dispatchResult = chppDispatchLoopbackServiceResponse(context, buf, len);
      break;
    }
#endif  // CHPP_CLIENT_ENABLED_LOOPBACK

#ifdef CHPP_CLIENT_ENABLED_TIMESYNC
    case CHPP_HANDLE_TIMESYNC: {
      dispatchResult = chppDispatchTimesyncServiceResponse(context, buf, len);
      break;
    }
#endif  // CHPP_CLIENT_ENABLED_TIMESYNC

#ifdef CHPP_CLIENT_ENABLED_DISCOVERY
    case CHPP_HANDLE_DISCOVERY: {
      dispatchResult = chppDispatchDiscoveryServiceResponse(context, buf, len);
      break;
    }
#endif  // CHPP_CLIENT_ENABLED_DISCOVERY

    default: {
      handleValid = false;
    }
  }

  if (dispatchResult == false) {
    CHPP_LOGE("Handle=%" PRIu8
              " received unknown service response. command=%#x, transaction ID="
              "%" PRIu8 ", len=%" PRIuSIZE,
              rxHeader->handle, rxHeader->command, rxHeader->transaction, len);
  }

  return handleValid;
}

/**
 * Processes a client notification that is determined to be for a predefined
 * CHPP service.
 *
 * @param context Maintains status for each app layer instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 *
 * @return False if handle is invalid. True otherwise.
 */
static bool chppProcessPredefinedClientNotification(
    struct ChppAppState *context, uint8_t *buf, size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;
  bool handleValid = true;
  bool dispatchResult = true;

  // No predefined services support these yet
  handleValid = false;

  UNUSED_VAR(context);
  UNUSED_VAR(len);
  UNUSED_VAR(rxHeader);
  UNUSED_VAR(dispatchResult);

  return handleValid;
}

/**
 * Processes a service notification that is determined to be for a predefined
 * CHPP client.
 *
 * @param context Maintains status for each app layer instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 *
 * @return False if handle is invalid. True otherwise.
 */
static bool chppProcessPredefinedServiceNotification(
    struct ChppAppState *context, uint8_t *buf, size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;
  bool handleValid = true;
  bool dispatchResult = true;

  // No predefined clients support these yet
  handleValid = false;

  UNUSED_VAR(context);
  UNUSED_VAR(len);
  UNUSED_VAR(rxHeader);
  UNUSED_VAR(dispatchResult);

  return handleValid;
}

/**
 * Verifies if the length of a Rx Datagram from the transport layer is
 * sufficient for the associated service.
 *
 * @param context Maintains status for each app layer instance.
 * @param rxHeader The pointer to the datagram RX header.
 * @param len Length of the datagram in bytes.
 *
 * @return true if length is ok.
 */
static bool chppDatagramLenIsOk(struct ChppAppState *context,
                                struct ChppAppHeader *rxHeader, size_t len) {
  size_t minLen = SIZE_MAX;
  uint8_t handle = rxHeader->handle;

  if (handle < CHPP_HANDLE_NEGOTIATED_RANGE_START) {  // Predefined
    switch (handle) {
      case CHPP_HANDLE_NONE:
        minLen = sizeof_member(struct ChppAppHeader, handle);
        break;

      case CHPP_HANDLE_LOOPBACK:
        minLen = sizeof_member(struct ChppAppHeader, handle) +
                 sizeof_member(struct ChppAppHeader, type);
        break;

      case CHPP_HANDLE_TIMESYNC:
      case CHPP_HANDLE_DISCOVERY:
        minLen = sizeof(struct ChppAppHeader);
        break;

      default:
        CHPP_LOGE("Invalid predefined handle %" PRIu8, handle);
    }

  } else {  // Negotiated
    enum ChppMessageType messageType =
        CHPP_APP_GET_MESSAGE_TYPE(rxHeader->type);
    switch (messageType) {
      case CHPP_MESSAGE_TYPE_CLIENT_REQUEST:
      case CHPP_MESSAGE_TYPE_CLIENT_NOTIFICATION: {
        minLen = chppServiceOfHandle(context, handle)->minLength;
        break;
      }
      case CHPP_MESSAGE_TYPE_SERVICE_RESPONSE:
      case CHPP_MESSAGE_TYPE_SERVICE_NOTIFICATION: {
        minLen = chppClientOfHandle(context, handle)->minLength;
        break;
      }
      default: {
        CHPP_LOGE("Invalid message type %d", messageType);
        break;
      }
    }
  }

  if (len < minLen) {
    CHPP_LOGE("Received datagram too short for handle=%" PRIu8
              ", len=%" PRIuSIZE " < %" PRIuSIZE,
              handle, len, minLen);
  }
  return (len >= minLen);
}

/**
 * Returns the dispatch function of a particular negotiated client/service
 * handle and message type. This shall be null if it is unsupported by the
 * service.
 *
 * @param context Maintains status for each app layer instance.
 * @param handle Handle number for the client/service.
 * @param type Message type.
 *
 * @return Pointer to a function that dispatches incoming datagrams for any
 * particular client/service.
 */
ChppDispatchFunction *chppGetDispatchFunction(struct ChppAppState *context,
                                              uint8_t handle,
                                              enum ChppMessageType type) {
  switch (CHPP_APP_GET_MESSAGE_TYPE(type)) {
    case CHPP_MESSAGE_TYPE_CLIENT_REQUEST: {
      return chppServiceOfHandle(context, handle)->requestDispatchFunctionPtr;
      break;
    }
    case CHPP_MESSAGE_TYPE_SERVICE_RESPONSE: {
      return chppClientOfHandle(context, handle)->responseDispatchFunctionPtr;
      break;
    }
    case CHPP_MESSAGE_TYPE_CLIENT_NOTIFICATION: {
      return chppServiceOfHandle(context, handle)
          ->notificationDispatchFunctionPtr;
      break;
    }
    case CHPP_MESSAGE_TYPE_SERVICE_NOTIFICATION: {
      return chppClientOfHandle(context, handle)
          ->notificationDispatchFunctionPtr;
      break;
    }
    default: {
      return NULL;
    }
  }
}

/**
 * Returns the reset function pointer of a particular negotiated client. The
 * function pointer will be set to null by clients that do not need or support
 * a reset notification.
 *
 * @param context Maintains status for each app layer instance.
 * @param index Index of the registered client.
 *
 * @return Pointer to the reset function.
 */
ChppResetNotifierFunction *chppGetClientResetNotifierFunction(
    struct ChppAppState *context, uint8_t index) {
  return context->registeredClients[index]->resetNotifierFunctionPtr;
}

/**
 * Returns the reset function pointer of a particular registered service. The
 * function pointer will be set to null by services that do not need or support
 * a reset notification.
 *
 * @param context Maintains status for each app layer instance.
 * @param index Index of the registered service.
 *
 * @return Pointer to the reset function.
 */
ChppResetNotifierFunction *chppGetServiceResetNotifierFunction(
    struct ChppAppState *context, uint8_t index) {
  return context->registeredServices[index]->resetNotifierFunctionPtr;
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
static inline const struct ChppService *chppServiceOfHandle(
    struct ChppAppState *context, uint8_t handle) {
  CHPP_DEBUG_ASSERT(CHPP_SERVICE_INDEX_OF_HANDLE(handle) <
                    context->registeredServiceCount);
  return context->registeredServices[CHPP_SERVICE_INDEX_OF_HANDLE(handle)];
}

/**
 * Returns a pointer to the ChppClient struct of a particular negotiated
 * handle. Returns null if a client doesn't exist for the handle.
 *
 * @param context Maintains status for each app layer instance.
 * @param handle Handle number for the service.
 *
 * @return Pointer to the ChppClient struct matched to a particular handle.
 */
static inline const struct ChppClient *chppClientOfHandle(
    struct ChppAppState *context, uint8_t handle) {
  CHPP_DEBUG_ASSERT(
      context->clientIndexOfServiceIndex[CHPP_SERVICE_INDEX_OF_HANDLE(handle)] <
      context->registeredClientCount);
  return context->registeredClients[context->clientIndexOfServiceIndex
                                        [CHPP_SERVICE_INDEX_OF_HANDLE(handle)]];
}

/**
 * Returns a pointer to the service struct of a particular negotiated service
 * handle.
 *
 * @param context Maintains status for each app layer instance.
 * @param handle Handle number for the service.
 *
 * @return Pointer to the context struct of the service.
 */
static inline void *chppServiceContextOfHandle(struct ChppAppState *context,
                                               uint8_t handle) {
  CHPP_DEBUG_ASSERT(CHPP_SERVICE_INDEX_OF_HANDLE(handle) <
                    context->registeredServiceCount);
  return context
      ->registeredServiceContexts[CHPP_SERVICE_INDEX_OF_HANDLE(handle)];
}

/**
 * Returns a pointer to the client struct of a particular negotiated client
 * handle.
 *
 * @param context Maintains status for each app layer instance.
 * @param handle Handle number for the service.
 *
 * @return Pointer to the ChppService struct of the client.
 */
static inline void *chppClientContextOfHandle(struct ChppAppState *context,
                                              uint8_t handle) {
  CHPP_DEBUG_ASSERT(CHPP_SERVICE_INDEX_OF_HANDLE(handle) <
                    context->registeredClientCount);
  return context
      ->registeredClientContexts[context->clientIndexOfServiceIndex
                                     [CHPP_SERVICE_INDEX_OF_HANDLE(handle)]];
}

/**
 * Returns a pointer to the client/service struct of a particular negotiated
 * client/service handle.
 *
 * @param appContext Maintains status for each app layer instance.
 * @param handle Handle number for the service.
 * @param type Message type (indicates if this is for a client or service).
 *
 * @return Pointer to the client/service struct of the service handle.
 */
static void *chppClientServiceContextOfHandle(struct ChppAppState *appContext,
                                              uint8_t handle,
                                              enum ChppMessageType type) {
  switch (CHPP_APP_GET_MESSAGE_TYPE(type)) {
    case CHPP_MESSAGE_TYPE_CLIENT_REQUEST:
    case CHPP_MESSAGE_TYPE_CLIENT_NOTIFICATION: {
      return chppServiceContextOfHandle(appContext, handle);
      break;
    }
    case CHPP_MESSAGE_TYPE_SERVICE_RESPONSE:
    case CHPP_MESSAGE_TYPE_SERVICE_NOTIFICATION: {
      return chppClientContextOfHandle(appContext, handle);
      break;
    }
    default: {
      CHPP_LOGE("Cannot provide context for unknown message type=0x%" PRIx8
                " (handle=%" PRIu8 ")",
                type, handle);
      return NULL;
    }
  }
}

/**
 * Processes a received datagram that is determined to be for a predefined CHPP
 * service. Responds with an error if unsuccessful.
 *
 * @param context Maintains status for each app layer instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppProcessPredefinedHandleDatagram(struct ChppAppState *context,
                                                uint8_t *buf, size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;
  bool success = true;

  switch (CHPP_APP_GET_MESSAGE_TYPE(rxHeader->type)) {
    case CHPP_MESSAGE_TYPE_CLIENT_REQUEST: {
      success = chppProcessPredefinedClientRequest(context, buf, len);
      break;
    }
    case CHPP_MESSAGE_TYPE_CLIENT_NOTIFICATION: {
      success = chppProcessPredefinedClientNotification(context, buf, len);
      break;
    }
    case CHPP_MESSAGE_TYPE_SERVICE_RESPONSE: {
      success = chppProcessPredefinedServiceResponse(context, buf, len);
      break;
    }
    case CHPP_MESSAGE_TYPE_SERVICE_NOTIFICATION: {
      success = chppProcessPredefinedServiceNotification(context, buf, len);
      break;
    }
    default: {
      success = false;
    }
  }

  if (success == false) {
    CHPP_LOGE("Predefined handle=%" PRIu8
              " does not support message type=0x%" PRIx8 " (len=%" PRIuSIZE
              ", transaction ID=%" PRIu8 ")",
              rxHeader->handle, rxHeader->type, len, rxHeader->transaction);
    chppEnqueueTxErrorDatagram(context->transportContext,
                               CHPP_TRANSPORT_ERROR_APPLAYER);
  }
}

/**
 * Processes a received datagram that is determined to be for a negotiated CHPP
 * service. Responds with an error if unsuccessful.
 *
 * @param context Maintains status for each app layer instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppProcessNegotiatedHandleDatagram(struct ChppAppState *context,
                                                uint8_t *buf, size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;
  enum ChppMessageType messageType = CHPP_APP_GET_MESSAGE_TYPE(rxHeader->type);

  void *clientServiceContext =
      chppClientServiceContextOfHandle(context, rxHeader->handle, messageType);
  if (clientServiceContext == NULL) {
    CHPP_LOGE("Negotiated handle=%" PRIu8 " for RX message type=0x%" PRIx8
              " is missing context (len=%" PRIuSIZE ", transaction ID=%" PRIu8
              ")",
              rxHeader->handle, rxHeader->type, len, rxHeader->transaction);
    chppEnqueueTxErrorDatagram(context->transportContext,
                               CHPP_TRANSPORT_ERROR_APPLAYER);
    CHPP_DEBUG_ASSERT(false);

  } else {
    ChppDispatchFunction *dispatchFunc =
        chppGetDispatchFunction(context, rxHeader->handle, messageType);
    if (dispatchFunc == NULL) {
      CHPP_LOGE("Negotiated handle=%" PRIu8
                " does not support RX message type=0x%" PRIx8 " (len=%" PRIuSIZE
                ", transaction ID=%" PRIu8 ")",
                rxHeader->handle, rxHeader->type, len, rxHeader->transaction);
      chppEnqueueTxErrorDatagram(context->transportContext,
                                 CHPP_TRANSPORT_ERROR_APPLAYER);

    } else {
      // All good. Dispatch datagram and possibly notify a waiting client

      enum ChppAppErrorCode error =
          dispatchFunc(clientServiceContext, buf, len);
      if (error != CHPP_APP_ERROR_NONE) {
        CHPP_LOGE("Dispatching RX datagram failed. error=0x%" PRIx16
                  " handle=0x%" PRIx8 ", type =0x%" PRIx8
                  ", transaction ID=%" PRIu8 ", command=0x%" PRIx16
                  ", len=%" PRIuSIZE,
                  error, rxHeader->handle, rxHeader->type,
                  rxHeader->transaction, rxHeader->command, len);

        // Only client requests require a dispatch failure response.
        if (messageType == CHPP_MESSAGE_TYPE_CLIENT_REQUEST) {
          struct ChppAppHeader *response =
              chppAllocServiceResponseFixed(rxHeader, struct ChppAppHeader);
          if (response == NULL) {
            CHPP_LOG_OOM();
          } else {
            response->error = CHPP_ATTR_AND_ERROR_TO_PACKET_CODE(
                CHPP_TRANSPORT_ATTR_NONE, error);
            chppEnqueueTxDatagramOrFail(context->transportContext, response,
                                        sizeof(*response));
          }
        }
      } else if (messageType == CHPP_MESSAGE_TYPE_SERVICE_RESPONSE) {
        // Datagram is a service response. Check for synchronous operation and
        // notify waiting client if needed.

        struct ChppClientState *clientContext =
            (struct ChppClientState *)clientServiceContext;
        chppMutexLock(&clientContext->responseMutex);
        clientContext->responseReady = true;
        CHPP_LOGD(
            "Finished dispatching a service response. Notifying a potential "
            "synchronous client");
        chppConditionVariableSignal(&clientContext->responseCondVar);
        chppMutexUnlock(&clientContext->responseMutex);
      }
    }
  }
}

/************************************************
 *  Public Functions
 ***********************************************/

void chppAppInit(struct ChppAppState *appContext,
                 struct ChppTransportState *transportContext) {
  // Default initialize all service/clients
  struct ChppClientServiceSet set;
  memset(&set, 0xff, sizeof(set));  // set all bits to 1

  chppAppInitWithClientServiceSet(appContext, transportContext, set);
}

void chppAppInitWithClientServiceSet(
    struct ChppAppState *appContext,
    struct ChppTransportState *transportContext,
    struct ChppClientServiceSet clientServiceSet) {
  CHPP_NOT_NULL(appContext);
  CHPP_NOT_NULL(transportContext);

  CHPP_LOGI("Initializing the CHPP app layer");

  // Don't reset entire ChppAppState to avoid clearing non-transient
  // contents e.g. discovery mutex/condvar/states.
  appContext->registeredServiceCount = 0;
  memset(appContext->registeredServices, 0,
         sizeof(appContext->registeredServices));
  memset(appContext->registeredServiceContexts, 0,
         sizeof(appContext->registeredServiceContexts));
  appContext->registeredClientCount = 0;
  memset(appContext->registeredClients, 0,
         sizeof(appContext->registeredClients));
  memset(appContext->registeredClientContexts, 0,
         sizeof(appContext->registeredClientContexts));
  memset(appContext->clientIndexOfServiceIndex, 0,
         sizeof(appContext->clientIndexOfServiceIndex));

  appContext->clientServiceSet = clientServiceSet;
  appContext->transportContext = transportContext;

#ifdef CHPP_CLIENT_ENABLED_DISCOVERY
  chppDiscoveryInit(appContext);
#endif  // CHPP_CLIENT_ENABLED_DISCOVERY

  chppPalSystemApiInit(appContext);
#ifdef CHPP_SERVICE_ENABLED
  chppRegisterCommonServices(appContext);
#endif
#ifdef CHPP_CLIENT_ENABLED
  chppRegisterCommonClients(appContext);
#endif
}

void chppAppDeinit(struct ChppAppState *appContext) {
  chppAppDeinitTransient(appContext);

#ifdef CHPP_CLIENT_ENABLED_DISCOVERY
  // Discovery should only be deinitialized on true CHPP app deinit
  // (shutdown), since a client may be waiting on discovery completion
  // during a transient deinit (reset).
  chppDiscoveryDeinit(appContext);
#endif  // CHPP_CLIENT_ENABLED_DISCOVERY
}

void chppAppDeinitTransient(struct ChppAppState *appContext) {
  CHPP_NOT_NULL(appContext);

  CHPP_LOGI("Deinitializing the CHPP app layer");

#ifdef CHPP_CLIENT_ENABLED
  chppDeregisterCommonClients(appContext);
#endif
#ifdef CHPP_SERVICE_ENABLED
  chppDeregisterCommonServices(appContext);
#endif
  chppPalSystemApiDeinit(appContext);
}

void chppAppProcessRxDatagram(struct ChppAppState *context, uint8_t *buf,
                              size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;

  if (len == 0) {
    CHPP_LOGE("chppAppProcessRxDatagram called with payload length of 0");
    CHPP_DEBUG_ASSERT(false);

  } else if (len < sizeof(struct ChppAppHeader)) {
    uint8_t *handle = (uint8_t *)buf;
    CHPP_LOGD("App layer RX datagram (len=%" PRIuSIZE ") for handle=%" PRIu8,
              len, *handle);

  } else {
    CHPP_LOGD("App layer RX datagram (len=%" PRIuSIZE ") for handle=%" PRIu8
              ", type=0x%" PRIx8 ", transaction ID=%" PRIu8 ", error=%" PRIu8
              ", command=0x%" PRIx16,
              len, rxHeader->handle, rxHeader->type, rxHeader->transaction,
              rxHeader->error, rxHeader->command);
  }

  if (chppDatagramLenIsOk(context, rxHeader, len)) {
    if (rxHeader->handle == CHPP_HANDLE_NONE) {
      chppDispatchNonHandle(context, buf, len);

    } else if (rxHeader->handle < CHPP_HANDLE_NEGOTIATED_RANGE_START) {
      chppProcessPredefinedHandleDatagram(context, buf, len);

    } else {
      chppProcessNegotiatedHandleDatagram(context, buf, len);
    }
  }

  chppDatagramProcessDoneCb(context->transportContext, buf);
}

void chppAppProcessRxReset(struct ChppAppState *context) {
  for (uint8_t i = 0; i < context->discoveredServiceCount; i++) {
    if (context->clientIndexOfServiceIndex[i] != CHPP_CLIENT_INDEX_NONE) {
      // Discovered service has a matched client
      ChppResetNotifierFunction *ResetNotifierFunction =
          chppGetClientResetNotifierFunction(
              context, context->clientIndexOfServiceIndex[i]);

      CHPP_LOGD(
          "Client # %" PRIu8 "(handle=%d) reset notifier %s",
          context->clientIndexOfServiceIndex[i],
          CHPP_SERVICE_HANDLE_OF_INDEX(i),
          (ResetNotifierFunction == NULL) ? "is unsupported" : "starting");

      if (ResetNotifierFunction != NULL) {
        ResetNotifierFunction(context);
      }
    }
  }

  for (uint8_t i = 0; i < context->registeredServiceCount; i++) {
    ChppResetNotifierFunction *ResetNotifierFunction =
        chppGetServiceResetNotifierFunction(context, i);

    CHPP_LOGD("Service # %" PRIu8 "(handle=%d) reset notifier %s", i,
              CHPP_SERVICE_HANDLE_OF_INDEX(i),
              (ResetNotifierFunction == NULL) ? "is unsupported" : "starting");

    if (ResetNotifierFunction != NULL) {
      ResetNotifierFunction(context);
    }
  }
}

void chppUuidToStr(const uint8_t uuid[CHPP_SERVICE_UUID_LEN],
                   char strOut[CHPP_SERVICE_UUID_STRING_LEN]) {
  snprintf(
      strOut, CHPP_SERVICE_UUID_STRING_LEN,
      "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
      uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7],
      uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14],
      uuid[15]);
}
