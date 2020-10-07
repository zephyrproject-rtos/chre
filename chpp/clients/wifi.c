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

#include "chpp/clients/wifi.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "chpp/app.h"
#include "chpp/clients.h"
#include "chpp/clients/discovery.h"
#include "chpp/common/standard_uuids.h"
#include "chpp/common/wifi.h"
#include "chpp/common/wifi_types.h"
#include "chpp/macros.h"
#include "chpp/memory.h"
#include "chpp/platform/log.h"
#include "chre/pal/wifi.h"
#include "chre_api/chre/wifi.h"

#ifndef CHPP_WIFI_DISCOVERY_TIMEOUT_MS
#define CHPP_WIFI_DISCOVERY_TIMEOUT_MS CHPP_DISCOVERY_DEFAULT_TIMEOUT_MS
#endif

/************************************************
 *  Prototypes
 ***********************************************/

static enum ChppAppErrorCode chppDispatchWifiResponse(void *clientContext,
                                                      uint8_t *buf, size_t len);
static enum ChppAppErrorCode chppDispatchWifiNotification(void *clientContext,
                                                          uint8_t *buf,
                                                          size_t len);
static bool chppWifiClientInit(void *clientContext, uint8_t handle,
                               struct ChppVersion serviceVersion);
static void chppWifiClientDeinit(void *clientContext);

/************************************************
 *  Private Definitions
 ***********************************************/

/**
 * Configuration parameters for this client
 */
static const struct ChppClient kWifiClientConfig = {
    .descriptor.uuid = CHPP_UUID_WIFI_STANDARD,

    // Version
    .descriptor.version.major = 1,
    .descriptor.version.minor = 0,
    .descriptor.version.patch = 0,

    // Server response dispatch function pointer
    .responseDispatchFunctionPtr = &chppDispatchWifiResponse,

    // Server notification dispatch function pointer
    .notificationDispatchFunctionPtr = &chppDispatchWifiNotification,

    // Server response dispatch function pointer
    .initFunctionPtr = &chppWifiClientInit,

    // Server notification dispatch function pointer
    .deinitFunctionPtr = &chppWifiClientDeinit,

    // Min length is the entire header
    .minLength = sizeof(struct ChppAppHeader),
};

/**
 * Structure to maintain state for the WiFi client and its Request/Response
 * (RR) functionality.
 */
struct ChppWifiClientState {
  struct ChppClientState client;     // WiFi client state
  const struct chrePalWifiApi *api;  // WiFi PAL API

  struct ChppRequestResponseState open;             // Service init state
  struct ChppRequestResponseState close;            // Service deinit state
  struct ChppRequestResponseState getCapabilities;  // Get Capabilities state
  struct ChppRequestResponseState
      configureScanMonitor;                     // Configure Scan Monitor state
  struct ChppRequestResponseState requestScan;  // Request Scan state
  struct ChppRequestResponseState requestRanging;  // Request Ranging state

  uint32_t capabilities;  // Cached GetCapabilities result
};

// Note: This global definition of gWifiClientContext supports only one
// instance of the CHPP WiFi client at a time.
struct ChppWifiClientState gWifiClientContext;
static const struct chrePalSystemApi *gSystemApi;
static const struct chrePalWifiCallbacks *gCallbacks;

/************************************************
 *  Prototypes
 ***********************************************/

static bool chppWifiClientOpen(const struct chrePalSystemApi *systemApi,
                               const struct chrePalWifiCallbacks *callbacks);
static void chppWifiClientClose(void);
static uint32_t chppWifiClientGetCapabilities(void);
static bool chppWifiClientConfigureScanMonitor(bool enable);
static bool chppWifiClientRequestScan(const struct chreWifiScanParams *params);
static void chppWifiClientReleaseScanEvent(struct chreWifiScanEvent *event);
static bool chppWifiClientRequestRanging(
    const struct chreWifiRangingParams *params);
static void chppWifiClientReleaseRangingEvent(
    struct chreWifiRangingEvent *event);

static void chppWifiOpenResult(struct ChppWifiClientState *clientContext,
                               uint8_t *buf, size_t len);
static void chppWifiCloseResult(struct ChppWifiClientState *clientContext,
                                uint8_t *buf, size_t len);
static void chppWifiGetCapabilitiesResult(
    struct ChppWifiClientState *clientContext, uint8_t *buf, size_t len);
static void chppWifiConfigureScanMonitorResult(
    struct ChppWifiClientState *clientContext, uint8_t *buf, size_t len);

static void chppWifiScanEventNotification(
    struct ChppWifiClientState *clientContext, uint8_t *buf, size_t len);
static void chppWifiRangingEventNotification(
    struct ChppWifiClientState *clientContext, uint8_t *buf, size_t len);

/************************************************
 *  Private Functions
 ***********************************************/

/**
 * Dispatches a server response from the transport layer that is determined to
 * be for the WiFi client.
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
static enum ChppAppErrorCode chppDispatchWifiResponse(void *clientContext,
                                                      uint8_t *buf,
                                                      size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;
  struct ChppWifiClientState *wifiClientContext =
      (struct ChppWifiClientState *)clientContext;
  enum ChppAppErrorCode error = CHPP_APP_ERROR_NONE;

  switch (rxHeader->command) {
    case CHPP_WIFI_OPEN: {
      chppClientTimestampResponse(&wifiClientContext->open, rxHeader);
      chppWifiOpenResult(wifiClientContext, buf, len);
      break;
    }

    case CHPP_WIFI_CLOSE: {
      chppClientTimestampResponse(&wifiClientContext->close, rxHeader);
      chppWifiCloseResult(wifiClientContext, buf, len);
      break;
    }

    case CHPP_WIFI_GET_CAPABILITIES: {
      chppClientTimestampResponse(&wifiClientContext->getCapabilities,
                                  rxHeader);
      chppWifiGetCapabilitiesResult(wifiClientContext, buf, len);
      break;
    }

    case CHPP_WIFI_CONFIGURE_SCAN_MONITOR_ASYNC: {
      chppClientTimestampResponse(&wifiClientContext->configureScanMonitor,
                                  rxHeader);
      chppWifiConfigureScanMonitorResult(wifiClientContext, buf, len);
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
 * Dispatches a server notification from the transport layer that is determined
 * to be for the WiFi client.
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
static enum ChppAppErrorCode chppDispatchWifiNotification(void *clientContext,
                                                          uint8_t *buf,
                                                          size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;
  struct ChppWifiClientState *wifiClientContext =
      (struct ChppWifiClientState *)clientContext;
  enum ChppAppErrorCode error = CHPP_APP_ERROR_NONE;

  switch (rxHeader->command) {
    case CHPP_WIFI_REQUEST_SCAN_ASYNC: {
      chppWifiScanEventNotification(wifiClientContext, buf, len);
      break;
    }

    case CHPP_WIFI_REQUEST_RANGING_ASYNC: {
      chppWifiRangingEventNotification(wifiClientContext, buf, len);
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
static bool chppWifiClientInit(void *clientContext, uint8_t handle,
                               struct ChppVersion serviceVersion) {
  UNUSED_VAR(serviceVersion);

  struct ChppWifiClientState *wifiClientContext =
      (struct ChppWifiClientState *)clientContext;
  chppClientInit(&wifiClientContext->client, handle);

  return true;
}

/**
 * Deinitializes the client.
 *
 * @param clientContext Maintains status for each client instance.
 */
static void chppWifiClientDeinit(void *clientContext) {
  struct ChppWifiClientState *wifiClientContext =
      (struct ChppWifiClientState *)clientContext;
  chppClientDeinit(&wifiClientContext->client);
}

/**
 * Handles the server response for the open client request.
 *
 * This function is called from chppDispatchWifiResponse().
 *
 * @param clientContext Maintains status for each client instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppWifiOpenResult(struct ChppWifiClientState *clientContext,
                               uint8_t *buf, size_t len) {
  // TODO
  UNUSED_VAR(clientContext);
  UNUSED_VAR(buf);
  UNUSED_VAR(len);
}

/**
 * Handles the server response for the close client request.
 *
 * This function is called from chppDispatchWifiResponse().
 *
 * @param clientContext Maintains status for each client instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppWifiCloseResult(struct ChppWifiClientState *clientContext,
                                uint8_t *buf, size_t len) {
  // TODO
  UNUSED_VAR(clientContext);
  UNUSED_VAR(buf);
  UNUSED_VAR(len);
}

/**
 * Handles the server response for the get capabilities client request.
 *
 * This function is called from chppDispatchWifiResponse().
 *
 * @param clientContext Maintains status for each client instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppWifiGetCapabilitiesResult(
    struct ChppWifiClientState *clientContext, uint8_t *buf, size_t len) {
  if (len < sizeof(struct ChppWifiGetCapabilitiesResponse)) {
    CHPP_LOGE("WiFi GetCapabilities result too short");

  } else {
    struct ChppWifiGetCapabilitiesParameters *result =
        &((struct ChppWifiGetCapabilitiesResponse *)buf)->params;

    CHPP_LOGD("chppWifiGetCapabilitiesResult received capabilities=0x%" PRIx32,
              result->capabilities);

    clientContext->capabilities = result->capabilities;
  }
}

/**
 * Handles the service response for the Configure Scan Monitor client request.
 *
 * This function is called from chppDispatchWifiResponse().
 *
 * @param clientContext Maintains status for each client instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppWifiConfigureScanMonitorResult(
    struct ChppWifiClientState *clientContext, uint8_t *buf, size_t len) {
  UNUSED_VAR(clientContext);

  if (len <
      sizeof(struct ChppWifiConfigureScanMonitorAsyncResponseParameters)) {
    CHPP_LOGE("WiFi ControlLocationSession result too short");

  } else {
    struct ChppWifiConfigureScanMonitorAsyncResponseParameters *result =
        (struct ChppWifiConfigureScanMonitorAsyncResponseParameters *)buf;

    CHPP_LOGD(
        "chppWifiConfigureScanMonitorResult received enable=%s, "
        "errorCode=%" PRIu8,
        result->enabled ? "true" : "false", result->errorCode);

    gCallbacks->scanMonitorStatusChangeCallback(result->enabled,
                                                result->errorCode);
  }
}

/**
 * Handles the WiFi scan event server notification.
 *
 * This function is called from chppDispatchWifiNotification().
 *
 * @param clientContext Maintains status for each client instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppWifiScanEventNotification(
    struct ChppWifiClientState *clientContext, uint8_t *buf, size_t len) {
  UNUSED_VAR(clientContext);
  CHPP_LOGD("chppWifiScanEventNotification received data len=%" PRIuSIZE, len);

  buf += sizeof(struct ChppAppHeader);
  len -= sizeof(struct ChppAppHeader);

  struct chreWifiScanEvent *chre =
      chppWifiScanEventToChre((struct ChppWifiScanEvent *)buf, len);

  if (chre == NULL) {
    CHPP_LOGE(
        "chppWifiScanEventNotification CHPP -> CHRE conversion failed. Input "
        "len=%" PRIuSIZE,
        len);
  } else {
    gCallbacks->scanEventCallback(chre);
  }
}

/**
 * Handles the WiFi ranging event server notification.
 *
 * This function is called from chppDispatchWifiNotification().
 *
 * @param clientContext Maintains status for each client instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppWifiRangingEventNotification(
    struct ChppWifiClientState *clientContext, uint8_t *buf, size_t len) {
  UNUSED_VAR(clientContext);
  UNUSED_VAR(buf);
  UNUSED_VAR(len);

  // TODO: Use auto-generated parser to convert, i.e.
  // chppWifiRangingEventToChre().
  // gCallbacks->rangingEventCallback(chreResult);
}

/**
 * Initializes the WiFi client upon an open request from CHRE and responds
 * with the result.
 *
 * @param systemApi CHRE system function pointers.
 * @param callbacks CHRE entry points.
 *
 * @return True if successful. False otherwise.
 */
static bool chppWifiClientOpen(const struct chrePalSystemApi *systemApi,
                               const struct chrePalWifiCallbacks *callbacks) {
  CHPP_DEBUG_ASSERT(systemApi != NULL);
  CHPP_DEBUG_ASSERT(callbacks != NULL);

  bool result = false;
  gSystemApi = systemApi;
  gCallbacks = callbacks;

  // Local
  gWifiClientContext.capabilities = CHRE_WIFI_CAPABILITIES_NONE;

  // Wait for discovery to complete for "open" call to succeed
  if (!chppWaitForDiscoveryComplete(gWifiClientContext.client.appContext,
                                    CHPP_WIFI_DISCOVERY_TIMEOUT_MS)) {
    CHPP_LOGE("Timed out waiting to discover CHPP WiFi service");
  } else {
    // Remote
    struct ChppAppHeader *request = chppAllocClientRequestCommand(
        &gWifiClientContext.client, CHPP_WIFI_OPEN);

    if (request == NULL) {
      CHPP_LOG_OOM();
    } else {
      // Send request and wait for service response
      result = chppSendTimestampedRequestAndWait(&gWifiClientContext.client,
                                                 &gWifiClientContext.open,
                                                 request, sizeof(*request));
    }
  }

  return result;
}

/**
 * Deinitializes the WiFi client.
 */
static void chppWifiClientClose(void) {
  // Remote
  struct ChppAppHeader *request = chppAllocClientRequestCommand(
      &gWifiClientContext.client, CHPP_WIFI_CLOSE);

  if (request == NULL) {
    CHPP_LOG_OOM();
  } else {
    chppSendTimestampedRequestAndWait(&gWifiClientContext.client,
                                      &gWifiClientContext.close, request,
                                      sizeof(*request));
  }
  // Local
  gWifiClientContext.capabilities = CHRE_WIFI_CAPABILITIES_NONE;
}

/**
 * Retrieves a set of flags indicating the WiFi features supported by the
 * current implementation.
 *
 * @return Capabilities flags.
 */
static uint32_t chppWifiClientGetCapabilities(void) {
  uint32_t capabilities = CHRE_WIFI_CAPABILITIES_NONE;

  if (gWifiClientContext.capabilities != CHRE_WIFI_CAPABILITIES_NONE) {
    // Result already cached
    capabilities = gWifiClientContext.capabilities;

  } else {
    struct ChppAppHeader *request = chppAllocClientRequestCommand(
        &gWifiClientContext.client, CHPP_WIFI_GET_CAPABILITIES);

    if (request == NULL) {
      CHPP_LOG_OOM();
    } else {
      if (chppSendTimestampedRequestAndWait(&gWifiClientContext.client,
                                            &gWifiClientContext.getCapabilities,
                                            request, sizeof(*request))) {
        // Success. gWifiClientContext.capabilities is now populated
        capabilities = gWifiClientContext.capabilities;
      }
    }
  }

  return capabilities;
}

/**
 * Enables/disables receiving unsolicited scan results.
 *
 * @param enable True to enable.
 *
 * @return True indicates the request was sent off to the service.
 */
static bool chppWifiClientConfigureScanMonitor(bool enable) {
  bool result = false;

  struct ChppWifiConfigureScanMonitorAsyncRequest *request =
      chppAllocClientRequestFixed(
          &gWifiClientContext.client,
          struct ChppWifiConfigureScanMonitorAsyncRequest);

  if (request == NULL) {
    CHPP_LOG_OOM();
  } else {
    request->header.command = CHPP_WIFI_CONFIGURE_SCAN_MONITOR_ASYNC;
    request->params.enable = enable;
    request->params.cookie = &gWifiClientContext.configureScanMonitor;

    result = chppSendTimestampedRequestOrFail(
        &gWifiClientContext.client, &gWifiClientContext.configureScanMonitor,
        request, sizeof(*request));
  }

  return result;
}

/**
 * Request that the WiFi chipset perform a scan or deliver results from its
 * cache.
 *
 * @param params See chreWifiRequestScanAsync().
 *
 * @return True indicates the request was sent off to the service.
 */
static bool chppWifiClientRequestScan(const struct chreWifiScanParams *params) {
  // TODO using parser, i.e. chppWifiScanParamsFromChre()
  UNUSED_VAR(params);

  return false;
}

/**
 * Releases the memory held for the scan event callback.
 *
 * @param event Location event to be released.
 */
static void chppWifiClientReleaseScanEvent(struct chreWifiScanEvent *event) {
  // TODO
  UNUSED_VAR(event);
}

/**
 * Request that the WiFi chipset perform RTT ranging.
 *
 * @param params See chreWifiRequestRangingAsync().
 *
 * @return True indicates the request was sent off to the service.
 */
static bool chppWifiClientRequestRanging(
    const struct chreWifiRangingParams *params) {
  // TODO
  UNUSED_VAR(params);

  return false;
}

/**
 * Releases the memory held for the RTT ranging event callback.
 *
 * @param event Location event to be released.
 */
static void chppWifiClientReleaseRangingEvent(
    struct chreWifiRangingEvent *event) {
  // TODO
  UNUSED_VAR(event);
}

/************************************************
 *  Public Functions
 ***********************************************/

void chppRegisterWifiClient(struct ChppAppState *appContext) {
  gWifiClientContext.client.appContext = appContext;
  chppRegisterClient(appContext, (void *)&gWifiClientContext,
                     &kWifiClientConfig);
}

void chppDeregisterWifiClient(struct ChppAppState *appContext) {
  // TODO

  UNUSED_VAR(appContext);
}

#ifdef CHPP_CLIENT_ENABLED_WIFI

#ifdef CHPP_CLIENT_ENABLED_CHRE_WIFI
const struct chrePalWifiApi *chrePalWifiGetApi(uint32_t requestedApiVersion) {
#else
const struct chrePalWifiApi *chppPalWifiGetApi(uint32_t requestedApiVersion) {
#endif

  static const struct chrePalWifiApi api = {
      .moduleVersion = CHPP_PAL_WIFI_API_VERSION,
      .open = chppWifiClientOpen,
      .close = chppWifiClientClose,
      .getCapabilities = chppWifiClientGetCapabilities,
      .configureScanMonitor = chppWifiClientConfigureScanMonitor,
      .requestScan = chppWifiClientRequestScan,
      .releaseScanEvent = chppWifiClientReleaseScanEvent,
      .requestRanging = chppWifiClientRequestRanging,
      .releaseRangingEvent = chppWifiClientReleaseRangingEvent,
  };

  CHPP_STATIC_ASSERT(
      CHRE_PAL_WIFI_API_CURRENT_VERSION == CHPP_PAL_WIFI_API_VERSION,
      "A newer CHRE PAL API version is available. Please update.");

  if (!CHRE_PAL_VERSIONS_ARE_COMPATIBLE(api.moduleVersion,
                                        requestedApiVersion)) {
    return NULL;
  } else {
    return &api;
  }
}

#endif
