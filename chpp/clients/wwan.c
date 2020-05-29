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
#include "chpp/app.h"
#include "chpp/common/wwan.h"
#include "chre/pal/wwan.h"

/************************************************
 *  Prototypes
 ***********************************************/

static bool chppDispatchWwanResponse(void *serviceContext, uint8_t *buf,
                                     size_t len);

/************************************************
 *  Private Definitions
 ***********************************************/

/**
 * Configuration parameters for this client
 */
static const struct ChppClient wwanClientConfig = {
    .descriptor.uuid = {0x0d, 0x0e, 0x0a, 0x0d, 0x0b, 0x0e, 0x0e, 0x0f, 0x0d,
                        0x0e, 0x0a, 0x0d, 0x0b, 0x0e, 0x0e, 0x0f},  // TODO

    .descriptor.versionMajor = 1,

    .descriptor.versionMinor = 0,

    .descriptor.versionPatch = 0,

    // Server response dispatch function pointer
    .responseDispatchFunctionPtr = &chppDispatchWwanResponse,

    // Server notification dispatch function pointer
    .notificationDispatchFunctionPtr = NULL,  // Not supported

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

void chppWwanOpenResult(struct ChppWwanClientState *clientContext, uint8_t *buf,
                        size_t len);
void chppWwanGetCapabilitiesResult(struct ChppWwanClientState *clientContext,
                                   uint8_t *buf, size_t len);
void chppWwanGetCellInfoAsyncResult(struct ChppWwanClientState *clientContext,
                                    uint8_t *buf, size_t len);

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
 * @return False indicates error (unknown command).
 */
static bool chppDispatchWwanResponse(void *clientContext, uint8_t *buf,
                                     size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;
  struct ChppWwanClientState *wwanClientContext =
      (struct ChppWwanClientState *)clientContext;
  bool success = true;

  switch (rxHeader->command) {
    case CHPP_WWAN_OPEN: {
      chppClientTimestampResponse(&wwanClientContext->open, rxHeader);
      chppWwanOpenResult(wwanClientContext, buf, len);
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
      success = false;
      break;
    }
  }

  return success;
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
void chppWwanOpenResult(struct ChppWwanClientState *clientContext, uint8_t *buf,
                        size_t len) {
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
void chppWwanGetCapabilitiesResult(struct ChppWwanClientState *clientContext,
                                   uint8_t *buf, size_t len) {
  // TODO
  UNUSED_VAR(clientContext);
  UNUSED_VAR(buf);
  UNUSED_VAR(len);

  // TODO: set clientContext->capabilities
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
void chppWwanGetCellInfoAsyncResult(struct ChppWwanClientState *clientContext,
                                    uint8_t *buf, size_t len) {
  UNUSED_VAR(clientContext);
  UNUSED_VAR(buf);
  UNUSED_VAR(len);
  // TODO: to be auto-generated by python e.g.
  // void *chreResult = decodeChppWwanCellInfoResultToChre();
  // gCallbacks->cellInfoResultCallback(chreResult);
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
bool chppWwanClientOpen(const struct chrePalSystemApi *systemApi,
                        const struct chrePalWwanCallbacks *callbacks) {
  gSystemApi = systemApi;
  gCallbacks = callbacks;

  // Local
  gWwanClientContext.capabilities = CHRE_WWAN_CAPABILITIES_NONE;

  // Remote
  struct ChppAppHeader *request =
      chppAllocClientRequestCommand(&gWwanClientContext.client, CHPP_WWAN_OPEN);

  chppSendTimestampedRequestOrFail(&gWwanClientContext.client,
                                   &gWwanClientContext.open, request,
                                   sizeof(struct ChppAppHeader));

  // TODO: Wait for response
  return true;
}

/**
 * Deinitializes the WWAN client.
 */
void chppWwanClientClose() {
  // Remote
  struct ChppAppHeader *request = chppAllocClientRequestCommand(
      &gWwanClientContext.client, CHPP_WWAN_CLOSE);

  chppSendTimestampedRequestOrFail(&gWwanClientContext.client,
                                   &gWwanClientContext.close, request,
                                   sizeof(struct ChppAppHeader));

  // Local
  gWwanClientContext.capabilities = CHRE_WWAN_CAPABILITIES_NONE;
}

/**
 * Retrieves a set of flags indicating the WWAN features supported by the
 * current implementation.
 *
 * @return Capabilities flags.
 */
uint32_t chppWwanClientGetCapabilities() {
  if (gWwanClientContext.capabilities != CHRE_WWAN_CAPABILITIES_NONE) {
    // Result already cached
    return gWwanClientContext.capabilities;

  } else {
    struct ChppAppHeader *request = chppAllocClientRequestCommand(
        &gWwanClientContext.client, CHPP_WWAN_GET_CAPABILITIES);

    if (chppSendTimestampedRequestOrFail(
            &gWwanClientContext.client, &gWwanClientContext.getCapabilities,
            request, sizeof(struct ChppAppHeader)) == false) {
      // Could not send out request
      return CHRE_WWAN_CAPABILITIES_NONE;

    } else {
      // TODO: Wait for response

      return gWwanClientContext.capabilities;
    }
  }
}

/**
 * Query information about the current serving cell and its neighbors. This does
 * not perform a network scan, but should return state from the current network
 * registration data stored in the cellular modem.
 *
 * @return True indicates the request was sent off to the service.
 */
bool chppWwanClientGetCellInfoAsync() {
  struct ChppAppHeader *request = chppAllocClientRequestCommand(
      &gWwanClientContext.client, CHPP_WWAN_GET_CELLINFO_ASYNC);

  return chppSendTimestampedRequestOrFail(
      &gWwanClientContext.client, &gWwanClientContext.getCellInfoAsync, request,
      sizeof(struct ChppAppHeader));
}

/**
 * Releases the memory held for the GetCellInfoAsync result.
 */
void chppWwanClientReleaseCellInfoResult() {
  // TODO
}

/************************************************
 *  Public Functions
 ***********************************************/

void chppRegisterWwanClient(struct ChppAppState *appContext) {
  // TODO

  UNUSED_VAR(appContext);
  UNUSED_VAR(wwanClientConfig);
}

#ifdef CHPP_CLIENT_ENABLED_WWAN
const struct chrePalWwanApi *chrePalWwanGetApi(uint32_t requestedApiVersion) {
  static const struct chrePalWwanApi api = {
      .moduleVersion = CHRE_PAL_WWAN_API_V1_4,
      .open = chppWwanClientOpen,
      .close = chppWwanClientClose,
      .getCapabilities = chppWwanClientGetCapabilities,
      .requestCellInfo = chppWwanClientGetCellInfoAsync,
      .releaseCellInfoResult = chppWwanClientReleaseCellInfoResult,
  };

  CHPP_STATIC_ASSERT(
      CHRE_PAL_WWAN_API_CURRENT_VERSION == CHRE_PAL_WWAN_API_V1_4,
      "A newer CHRE PAL API version is available, consider updating this file");

  if (!CHRE_PAL_VERSIONS_ARE_COMPATIBLE(api.moduleVersion,
                                        requestedApiVersion)) {
    return NULL;
  } else {
    return &api;
  }
}
#endif