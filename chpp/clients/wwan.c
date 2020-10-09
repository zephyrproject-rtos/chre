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

#include "chpp/clients/wwan.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "chpp/app.h"
#include "chpp/clients/discovery.h"
#include "chpp/common/standard_uuids.h"
#include "chpp/common/wwan.h"
#include "chpp/common/wwan_types.h"
#include "chpp/log.h"
#include "chpp/macros.h"
#include "chre/pal/wwan.h"

#ifndef CHPP_WWAN_DISCOVERY_TIMEOUT_MS
#define CHPP_WWAN_DISCOVERY_TIMEOUT_MS CHPP_DISCOVERY_DEFAULT_TIMEOUT_MS
#endif

/************************************************
 *  Prototypes
 ***********************************************/

static enum ChppAppErrorCode chppDispatchWwanResponse(void *clientContext,
                                                      uint8_t *buf, size_t len);
static bool chppWwanClientInit(void *clientContext, uint8_t handle,
                               struct ChppVersion serviceVersion);
static void chppWwanClientDeinit(void *clientContext);

/************************************************
 *  Private Definitions
 ***********************************************/

/**
 * Configuration parameters for this client
 */
static const struct ChppClient kWwanClientConfig = {
    .descriptor.uuid = CHPP_UUID_WWAN_STANDARD,

    // Version
    .descriptor.version.major = 1,
    .descriptor.version.minor = 0,
    .descriptor.version.patch = 0,

    // Server response dispatch function pointer
    .responseDispatchFunctionPtr = &chppDispatchWwanResponse,

    // Server notification dispatch function pointer
    .notificationDispatchFunctionPtr = NULL,  // Not supported

    // Server response dispatch function pointer
    .initFunctionPtr = &chppWwanClientInit,

    // Server notification dispatch function pointer
    .deinitFunctionPtr = &chppWwanClientDeinit,

    // Min length is the entire header
    .minLength = sizeof(struct ChppAppHeader),
};

/**
 * Structure to maintain state for the WWAN client and its Request/Response
 * (RR) functionality.
 */
struct ChppWwanClientState {
  struct ChppClientState client;     // WWAN client state
  const struct chrePalWwanApi *api;  // WWAN PAL API

  struct ChppRequestResponseState open;              // Service init state
  struct ChppRequestResponseState close;             // Service deinit state
  struct ChppRequestResponseState getCapabilities;   // Get Capabilities state
  struct ChppRequestResponseState getCellInfoAsync;  // Get CellInfo Async state

  uint32_t capabilities;  // Cached GetCapabilities result
};

// Note: This global definition of gWwanClientContext supports only one
// instance of the CHPP WWAN client at a time.
struct ChppWwanClientState gWwanClientContext;
static const struct chrePalSystemApi *gSystemApi;
static const struct chrePalWwanCallbacks *gCallbacks;

/************************************************
 *  Prototypes
 ***********************************************/

static bool chppWwanClientOpen(const struct chrePalSystemApi *systemApi,
                               const struct chrePalWwanCallbacks *callbacks);
static void chppWwanClientClose(void);
static uint32_t chppWwanClientGetCapabilities(void);
static bool chppWwanClientGetCellInfoAsync(void);
static void chppWwanClientReleaseCellInfoResult(
    struct chreWwanCellInfoResult *result);

static void chppWwanOpenResult(struct ChppWwanClientState *clientContext,
                               uint8_t *buf, size_t len);
static void chppWwanCloseResult(struct ChppWwanClientState *clientContext,
                                uint8_t *buf, size_t len);
static void chppWwanGetCapabilitiesResult(
    struct ChppWwanClientState *clientContext, uint8_t *buf, size_t len);
static void chppWwanGetCellInfoAsyncResult(
    struct ChppWwanClientState *clientContext, uint8_t *buf, size_t len);

/************************************************
 *  Private Functions
 ***********************************************/

/**
 * Dispatches a server response from the transport layer that is determined to
 * be for the WWAN client.
 *
 * This function is called from the app layer using its function pointer given
 * during client registration.
 *
 * @param clientContext Maintains status for each client instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 *
 * @return Indicates the result of this function call.
 */
static enum ChppAppErrorCode chppDispatchWwanResponse(void *clientContext,
                                                      uint8_t *buf,
                                                      size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;
  struct ChppWwanClientState *wwanClientContext =
      (struct ChppWwanClientState *)clientContext;
  enum ChppAppErrorCode error = CHPP_APP_ERROR_NONE;

  switch (rxHeader->command) {
    case CHPP_WWAN_OPEN: {
      chppClientTimestampResponse(&wwanClientContext->open, rxHeader);
      chppWwanOpenResult(wwanClientContext, buf, len);
      break;
    }

    case CHPP_WWAN_CLOSE: {
      chppClientTimestampResponse(&wwanClientContext->close, rxHeader);
      chppWwanCloseResult(wwanClientContext, buf, len);
      break;
    }

    case CHPP_WWAN_GET_CAPABILITIES: {
      chppClientTimestampResponse(&wwanClientContext->getCapabilities,
                                  rxHeader);
      chppWwanGetCapabilitiesResult(wwanClientContext, buf, len);
      break;
    }

    case CHPP_WWAN_GET_CELLINFO_ASYNC: {
      chppClientTimestampResponse(&wwanClientContext->getCellInfoAsync,
                                  rxHeader);
      chppWwanGetCellInfoAsyncResult(wwanClientContext, buf, len);
      break;
    }

    default: {
      error = CHPP_APP_ERROR_INVALID_COMMAND;
      break;
    }
  }

  return error;
}

/**
 * Initializes the client and provides its handle number and the version of the
 * matched service when/if it the client is matched with a service during
 * discovery.
 *
 * @param clientContext Maintains status for each client instance.
 * @param handle Handle number for this client.
 * @param serviceVersion Version of the matched service.
 *
 * @return True if client is compatible and successfully initialized.
 */
static bool chppWwanClientInit(void *clientContext, uint8_t handle,
                               struct ChppVersion serviceVersion) {
  UNUSED_VAR(serviceVersion);

  struct ChppWwanClientState *wwanClientContext =
      (struct ChppWwanClientState *)clientContext;
  chppClientInit(&wwanClientContext->client, handle);

  return true;
}

/**
 * Deinitializes the client.
 *
 * @param clientContext Maintains status for each client instance.
 */
static void chppWwanClientDeinit(void *clientContext) {
  struct ChppWwanClientState *wwanClientContext =
      (struct ChppWwanClientState *)clientContext;
  chppClientDeinit(&wwanClientContext->client);
}

/**
 * Handles the server response for the open client request.
 *
 * This function is called from chppDispatchWwanResponse().
 *
 * @param clientContext Maintains status for each client instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppWwanOpenResult(struct ChppWwanClientState *clientContext,
                               uint8_t *buf, size_t len) {
  // TODO
  UNUSED_VAR(clientContext);
  UNUSED_VAR(buf);
  UNUSED_VAR(len);
}

/**
 * Handles the server response for the close client request.
 *
 * This function is called from chppDispatchWwanResponse().
 *
 * @param clientContext Maintains status for each client instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppWwanCloseResult(struct ChppWwanClientState *clientContext,
                                uint8_t *buf, size_t len) {
  // TODO
  UNUSED_VAR(clientContext);
  UNUSED_VAR(buf);
  UNUSED_VAR(len);
}

/**
 * Handles the server response for the get capabilities client request.
 *
 * This function is called from chppDispatchWwanResponse().
 *
 * @param clientContext Maintains status for each client instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppWwanGetCapabilitiesResult(
    struct ChppWwanClientState *clientContext, uint8_t *buf, size_t len) {
  if (len < sizeof(struct ChppWwanGetCapabilitiesResponse)) {
    CHPP_LOGE("WWAN GetCapabilities result too short");

  } else {
    struct ChppWwanGetCapabilitiesParameters *result =
        &((struct ChppWwanGetCapabilitiesResponse *)buf)->params;

    CHPP_LOGD("chppWwanGetCapabilitiesResult received capabilities=0x%" PRIx32,
              result->capabilities);

    clientContext->capabilities = result->capabilities;
  }
}

/**
 * Handles the server response for the asynchronous get cell info client
 * request.
 *
 * This function is called from chppDispatchWwanResponse().
 *
 * @param clientContext Maintains status for each client instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppWwanGetCellInfoAsyncResult(
    struct ChppWwanClientState *clientContext, uint8_t *buf, size_t len) {
  UNUSED_VAR(clientContext);
  CHPP_LOGD("chppWwanGetCellInfoAsyncResult received data len=%" PRIuSIZE, len);

  buf += sizeof(struct ChppAppHeader);
  len -= sizeof(struct ChppAppHeader);

  struct chreWwanCellInfoResult *chre =
      chppWwanCellInfoResultToChre((struct ChppWwanCellInfoResult *)buf, len);

  if (chre == NULL) {
    CHPP_LOGE(
        "chppWwanGetCellInfoAsyncResult CHPP -> CHRE conversion failed. Input "
        "len=%" PRIuSIZE,
        len);
  } else {
    gCallbacks->cellInfoResultCallback(chre);
  }
}

/**
 * Initializes the WWAN client upon an open request from CHRE and responds with
 * the result.
 *
 * @param systemApi CHRE system function pointers.
 * @param callbacks CHRE entry points.
 *
 * @return True if successful. False otherwise.
 */
static bool chppWwanClientOpen(const struct chrePalSystemApi *systemApi,
                               const struct chrePalWwanCallbacks *callbacks) {
  CHPP_DEBUG_ASSERT(systemApi != NULL);
  CHPP_DEBUG_ASSERT(callbacks != NULL);

  bool result = false;
  gSystemApi = systemApi;
  gCallbacks = callbacks;

  // Local
  gWwanClientContext.capabilities = CHRE_WWAN_CAPABILITIES_NONE;

  // Wait for discovery to complete for "open" call to succeed
  if (!chppWaitForDiscoveryComplete(gWwanClientContext.client.appContext,
                                    CHPP_WWAN_DISCOVERY_TIMEOUT_MS)) {
    CHPP_LOGE("Timed out waiting to discover CHPP WWAN service");
  } else {
    // Remote
    struct ChppAppHeader *request = chppAllocClientRequestCommand(
        &gWwanClientContext.client, CHPP_WWAN_OPEN);

    if (request == NULL) {
      CHPP_LOG_OOM();
    } else {
      // Send request and wait for service response
      result = chppSendTimestampedRequestAndWait(&gWwanClientContext.client,
                                                 &gWwanClientContext.open,
                                                 request, sizeof(*request));
    }
  }

  return result;
}

/**
 * Deinitializes the WWAN client.
 */
static void chppWwanClientClose(void) {
  // Remote
  struct ChppAppHeader *request = chppAllocClientRequestCommand(
      &gWwanClientContext.client, CHPP_WWAN_CLOSE);

  if (request == NULL) {
    CHPP_LOG_OOM();
  } else {
    chppSendTimestampedRequestAndWait(&gWwanClientContext.client,
                                      &gWwanClientContext.close, request,
                                      sizeof(*request));
  }
  // Local
  gWwanClientContext.capabilities = CHRE_WWAN_CAPABILITIES_NONE;
}

/**
 * Retrieves a set of flags indicating the WWAN features supported by the
 * current implementation.
 *
 * @return Capabilities flags.
 */
static uint32_t chppWwanClientGetCapabilities(void) {
  uint32_t capabilities = CHRE_WWAN_CAPABILITIES_NONE;

  if (gWwanClientContext.capabilities != CHRE_WWAN_CAPABILITIES_NONE) {
    // Result already cached
    capabilities = gWwanClientContext.capabilities;

  } else {
    struct ChppAppHeader *request = chppAllocClientRequestCommand(
        &gWwanClientContext.client, CHPP_WWAN_GET_CAPABILITIES);

    if (request == NULL) {
      CHPP_LOG_OOM();
    } else {
      if (chppSendTimestampedRequestAndWait(&gWwanClientContext.client,
                                            &gWwanClientContext.getCapabilities,
                                            request, sizeof(*request))) {
        // Success. gWwanClientContext.capabilities is now populated
        capabilities = gWwanClientContext.capabilities;
      }
    }
  }

  return capabilities;
}

/**
 * Query information about the current serving cell and its neighbors. This does
 * not perform a network scan, but should return state from the current network
 * registration data stored in the cellular modem.
 *
 * @return True indicates the request was sent off to the service.
 */
static bool chppWwanClientGetCellInfoAsync(void) {
  bool result = false;

  struct ChppAppHeader *request = chppAllocClientRequestCommand(
      &gWwanClientContext.client, CHPP_WWAN_GET_CELLINFO_ASYNC);

  if (request == NULL) {
    CHPP_LOG_OOM();
  } else {
    result = chppSendTimestampedRequestOrFail(
        &gWwanClientContext.client, &gWwanClientContext.getCellInfoAsync,
        request, sizeof(*request));
  }

  return result;
}

/**
 * Releases the memory held for the GetCellInfoAsync result.
 */
static void chppWwanClientReleaseCellInfoResult(
    struct chreWwanCellInfoResult *result) {
  // TODO
  UNUSED_VAR(result);
}

/************************************************
 *  Public Functions
 ***********************************************/

void chppRegisterWwanClient(struct ChppAppState *appContext) {
  gWwanClientContext.client.appContext = appContext;
  chppRegisterClient(appContext, (void *)&gWwanClientContext,
                     &kWwanClientConfig);
}

void chppDeregisterWwanClient(struct ChppAppState *appContext) {
  // TODO

  UNUSED_VAR(appContext);
}

#ifdef CHPP_CLIENT_ENABLED_WWAN

#ifdef CHPP_CLIENT_ENABLED_CHRE_WWAN
const struct chrePalWwanApi *chrePalWwanGetApi(uint32_t requestedApiVersion) {
#else
const struct chrePalWwanApi *chppPalWwanGetApi(uint32_t requestedApiVersion) {
#endif

  static const struct chrePalWwanApi api = {
      .moduleVersion = CHPP_PAL_WWAN_API_VERSION,
      .open = chppWwanClientOpen,
      .close = chppWwanClientClose,
      .getCapabilities = chppWwanClientGetCapabilities,
      .requestCellInfo = chppWwanClientGetCellInfoAsync,
      .releaseCellInfoResult = chppWwanClientReleaseCellInfoResult,
  };

  CHPP_STATIC_ASSERT(
      CHRE_PAL_WWAN_API_CURRENT_VERSION == CHPP_PAL_WWAN_API_VERSION,
      "A newer CHRE PAL API version is available. Please update.");

  if (!CHRE_PAL_VERSIONS_ARE_COMPATIBLE(api.moduleVersion,
                                        requestedApiVersion)) {
    return NULL;
  } else {
    return &api;
  }
}

#endif
