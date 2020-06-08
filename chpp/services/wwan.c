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

#include "chpp/services/wwan.h"

#include "chpp/common/wwan.h"
#include "chre/pal/wwan.h"

/************************************************
 *  Prototypes
 ***********************************************/

static bool chppDispatchWwanRequest(void *serviceContext, uint8_t *buf,
                                    size_t len);

/************************************************
 *  Private Definitions
 ***********************************************/

/**
 * Configuration parameters for this service
 */
static const struct ChppService wwanServiceConfig = {
    .descriptor.uuid = {0x0d, 0x0e, 0x0a, 0x0d, 0x0b, 0x0e, 0x0e, 0x0f, 0x0d,
                        0x0e, 0x0a, 0x0d, 0x0b, 0x0e, 0x0e, 0x0f},  // TODO

    // Human-readable name
    .descriptor.name = "WWAN",

    // Version
    .descriptor.versionMajor = 1,
    .descriptor.versionMinor = 0,
    .descriptor.versionPatch = 0,

    // Client request dispatch function pointer
    .requestDispatchFunctionPtr = &chppDispatchWwanRequest,

    // Client notification dispatch function pointer
    .notificationDispatchFunctionPtr = NULL,  // Not supported

    // Min length is the entire header
    .minLength = sizeof(struct ChppAppHeader),
};

/**
 * Structure to maintain state for the WWAN service and its Request/Response
 * (RR) functionality.
 */
struct ChppWwanServiceState {
  struct ChppServiceState service;   // WWAN service state
  const struct chrePalWwanApi *api;  // WWAN PAL API

  struct ChppRequestResponseState open;              // Service init state
  struct ChppRequestResponseState close;             // Service deinit state
  struct ChppRequestResponseState getCapabilities;   // Get Capabilities state
  struct ChppRequestResponseState getCellInfoAsync;  // Get CellInfo Async state
};

// Note: This global definition of gWwanServiceContext supports only one
// instance of the CHPP WWAN service at a time. This limitation is primarily due
// to the PAL API.
// It would be possible to generate different API and callback pointers to
// support multiple instances of the service or modify the PAL API to pass a
// void* for context, but this is not necessary in the current version of CHPP.
// In such case, wwanServiceContext would be allocated dynamically as part of
// chppRegisterWwanService(), e.g.
//   struct ChppWwanServiceState *wwanServiceContext = chppMalloc(...);
// instead of globally here.
struct ChppWwanServiceState gWwanServiceContext;

/************************************************
 *  Prototypes
 ***********************************************/

static void chppWwanServiceOpen(struct ChppWwanServiceState *wwanServiceContext,
                                struct ChppAppHeader *requestHeader);
static void chppWwanServiceClose(
    struct ChppWwanServiceState *wwanServiceContext,
    struct ChppAppHeader *requestHeader);
static void chppWwanServiceGetCapabilities(
    struct ChppWwanServiceState *wwanServiceContext,
    struct ChppAppHeader *requestHeader);
static void chppWwanServiceGetCellInfoAsync(
    struct ChppWwanServiceState *wwanServiceContext,
    struct ChppAppHeader *requestHeader);
static void chppWwanServiceCellInfoResultCallback(
    struct chreWwanCellInfoResult *result);

/************************************************
 *  Private Functions
 ***********************************************/

/**
 * Dispatches a client request from the transport layer that is determined to be
 * for the WWAN service.
 *
 * This function is called from the app layer using its function pointer given
 * during service registration.
 *
 * @param serviceContext Maintains status for each service instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 *
 * @return False indicates error (unknown command).
 */
static bool chppDispatchWwanRequest(void *serviceContext, uint8_t *buf,
                                    size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;
  struct ChppWwanServiceState *wwanServiceContext =
      (struct ChppWwanServiceState *)serviceContext;
  bool success = true;

  UNUSED_VAR(len);

  switch (rxHeader->command) {
    case CHPP_WWAN_OPEN: {
      chppServiceTimestampRequest(&wwanServiceContext->open, rxHeader);
      chppWwanServiceOpen(wwanServiceContext, rxHeader);
      break;
    }

    case CHPP_WWAN_CLOSE: {
      chppServiceTimestampRequest(&wwanServiceContext->close, rxHeader);
      chppWwanServiceClose(wwanServiceContext, rxHeader);
      break;
    }

    case CHPP_WWAN_GET_CAPABILITIES: {
      chppServiceTimestampRequest(&wwanServiceContext->getCapabilities,
                                  rxHeader);
      chppWwanServiceGetCapabilities(wwanServiceContext, rxHeader);
      break;
    }

    case CHPP_WWAN_GET_CELLINFO_ASYNC: {
      chppServiceTimestampRequest(&wwanServiceContext->getCellInfoAsync,
                                  rxHeader);
      chppWwanServiceGetCellInfoAsync(wwanServiceContext, rxHeader);
      break;
    }

    default: {
      success = false;
      break;
    }
  }

  return success;
}

/**
 * Initializes the WWAN service upon an open request from the client and
 * responds to the client with the result.
 *
 * @param serviceContext Maintains status for each service instance.
 * @param requestHeader App layer header of the request.
 */
static void chppWwanServiceOpen(struct ChppWwanServiceState *wwanServiceContext,
                                struct ChppAppHeader *requestHeader) {
  // Allocate the response
  struct ChppServiceBasicResponse *response = chppAllocServiceResponseFixed(
      requestHeader, struct ChppServiceBasicResponse);

  // Initialize service
  static const struct chrePalWwanCallbacks palCallbacks = {
      .cellInfoResultCallback = chppWwanServiceCellInfoResultCallback,
  };

  if (!wwanServiceContext->api->open(
          wwanServiceContext->service.appContext->systemApi, &palCallbacks)) {
    // initialization failed
    CHPP_LOGE("WWAN PAL API initialization failed");
    CHPP_DEBUG_ASSERT(false);
    response->error = CHPP_APP_ERROR_UNSPECIFIED;
  } else {
    response->error = CHPP_APP_ERROR_NONE;
  }

  // Timestamp and send out response datagram
  chppSendTimestampedResponseOrFail(&wwanServiceContext->service,
                                    &wwanServiceContext->open, response,
                                    sizeof(*response));
}

/**
 * Deinitializes the WWAN service.
 *
 * @param serviceContext Maintains status for each service instance.
 * @param requestHeader App layer header of the request.
 */
static void chppWwanServiceClose(
    struct ChppWwanServiceState *wwanServiceContext,
    struct ChppAppHeader *requestHeader) {
  // Allocate the response
  struct ChppServiceBasicResponse *response = chppAllocServiceResponseFixed(
      requestHeader, struct ChppServiceBasicResponse);

  // Deinitialize service
  wwanServiceContext->api->close();

  // Timestamp and send out response datagram
  response->error = CHPP_APP_ERROR_NONE;
  chppSendTimestampedResponseOrFail(&wwanServiceContext->service,
                                    &wwanServiceContext->close, response,
                                    sizeof(*response));
}

/**
 * Retrieves a set of flags indicating the WWAN features supported by the
 * current implementation.
 *
 * @param serviceContext Maintains status for each service instance.
 * @param requestHeader App layer header of the request.
 */
static void chppWwanServiceGetCapabilities(
    struct ChppWwanServiceState *wwanServiceContext,
    struct ChppAppHeader *requestHeader) {
  // Allocate the response
  struct ChppWwanGetCapabilitiesResponse *response =
      chppAllocServiceResponseFixed(requestHeader,
                                    struct ChppWwanGetCapabilitiesResponse);

  // Populate the response
  response->capabilities = wwanServiceContext->api->getCapabilities();
  response->common.error = CHPP_APP_ERROR_NONE;

  // Timestamp and send out response datagram
  chppSendTimestampedResponseOrFail(&wwanServiceContext->service,
                                    &wwanServiceContext->getCapabilities,
                                    response, sizeof(*response));
}

/**
 * Query information about the current serving cell and its neighbors in
 * response to a client request. This does not perform a network scan, but
 * should return state from the current network registration data stored in the
 * cellular modem.
 *
 * This function returns an error code synchronously. The requested cellular
 * information shall be returned asynchronously to the client via the
 * chppPlatformWwanCellInfoResultEvent() service response.
 *
 * @param serviceContext Maintains status for each service instance.
 * @param requestHeader App layer header of the request.
 */
static void chppWwanServiceGetCellInfoAsync(
    struct ChppWwanServiceState *wwanServiceContext,
    struct ChppAppHeader *requestHeader) {
  // Register for callback
  if (!wwanServiceContext->api->requestCellInfo()) {
    // Error occurred, send a synchronous error response
    struct ChppServiceBasicResponse *response = chppAllocServiceResponseFixed(
        requestHeader, struct ChppServiceBasicResponse);
    response->error = CHPP_APP_ERROR_UNSPECIFIED;
    chppSendTimestampedResponseOrFail(&wwanServiceContext->service,
                                      &wwanServiceContext->getCellInfoAsync,
                                      response, sizeof(*response));
  }
}

static void chppWwanServiceCellInfoResultCallback(
    struct chreWwanCellInfoResult *result) {
  // Recover state
  struct ChppRequestResponseState *rRState =
      (struct ChppRequestResponseState *)result->cookie;
  struct ChppWwanServiceState *wwanServiceContext =
      container_of(rRState, struct ChppWwanServiceState, getCellInfoAsync);

  // Recreate request header
  struct ChppAppHeader requestHeader = {
      .handle = wwanServiceContext->service.handle,
      .transaction = rRState->transaction,
      .command = CHPP_WWAN_GET_CELLINFO_ASYNC,
  };

  // Craft response per parser script
  // TODO, this is a placeholder
  size_t responseLen = sizeof(struct ChppServiceBasicResponse) + 0 + 0;
  struct ChppServiceBasicResponse *response = chppAllocServiceResponseFixed(
      &requestHeader, struct ChppServiceBasicResponse);

  if (response == NULL) {
    CHPP_LOG_OOM("WwanGetCellInfoResponseAsync response of %zu bytes",
                 responseLen);
    CHPP_ASSERT(false);
  }

  // Populate remaining parts of response per parser script
  // TODO, this is a placeholder
  response->error = CHPP_APP_ERROR_NONE;

  // Timestamp and send out response datagram
  chppSendTimestampedResponseOrFail(&wwanServiceContext->service, rRState,
                                    response, responseLen);

  gWwanServiceContext.api->releaseCellInfoResult(result);
}

/************************************************
 *  Public Functions
 ***********************************************/

void chppRegisterWwanService(struct ChppAppState *appContext) {
  // Initialize platform
  gWwanServiceContext.api = chrePalWwanGetApi(CHRE_PAL_WWAN_API_V1_4);
  if (gWwanServiceContext.api == NULL) {
    CHPP_LOGE(
        "WWAN PAL API version not compatible with CHPP. Cannot register WWAN "
        "service");
    CHPP_DEBUG_ASSERT(false);
  } else {
    // Register service
    gWwanServiceContext.service.appContext = appContext;
    gWwanServiceContext.service.handle = chppRegisterService(
        appContext, (void *)&gWwanServiceContext, &wwanServiceConfig);
    CHPP_DEBUG_ASSERT(gWwanServiceContext.service.handle);
  }
}

void chppDeregisterWwanService(struct ChppAppState *appContext) {
  // TODO

  UNUSED_VAR(appContext);
}