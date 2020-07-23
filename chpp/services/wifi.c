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

#include "chpp/services/wifi.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include "chpp/common/standard_uuids.h"
#include "chpp/common/wifi.h"
#include "chpp/macros.h"
#include "chpp/platform/log.h"
#include "chpp/services.h"
#include "chpp/services/wifi_types.h"
#include "chre/pal/wifi.h"

/************************************************
 *  Prototypes
 ***********************************************/

static bool chppDispatchWifiRequest(void *serviceContext, uint8_t *buf,
                                    size_t len);

/************************************************
 *  Private Definitions
 ***********************************************/

/**
 * Configuration parameters for this service
 */
static const struct ChppService kWifiServiceConfig = {
    .descriptor.uuid = CHPP_UUID_WIFI_STANDARD,

    // Human-readable name
    .descriptor.name = "WiFi",

    // Version
    .descriptor.version.major = 1,
    .descriptor.version.minor = 0,
    .descriptor.version.patch = 0,

    // Client request dispatch function pointer
    .requestDispatchFunctionPtr = &chppDispatchWifiRequest,

    // Client notification dispatch function pointer
    .notificationDispatchFunctionPtr = NULL,  // Not supported

    // Min length is the entire header
    .minLength = sizeof(struct ChppAppHeader),
};

/**
 * Structure to maintain state for the WiFi service and its Request/Response
 * (RR) functionality.
 */
struct ChppWifiServiceState {
  struct ChppServiceState service;   // WiFi service state
  const struct chrePalWifiApi *api;  // WiFi PAL API

  // Based on chre/pal/wifi.h and chrePalWifiApi
  struct ChppRequestResponseState open;             // Service init state
  struct ChppRequestResponseState close;            // Service deinit state
  struct ChppRequestResponseState getCapabilities;  // Get Capabilities state
  struct ChppRequestResponseState
      configureScanMonitorAsync;  // Configure scan monitor state
  struct ChppRequestResponseState requestScanAsync;     // Request scan state
  struct ChppRequestResponseState requestRangingAsync;  // Request ranging state
};

// Note: The CHRE PAL API only allows for one definition - see comment in WWAN
// service for details.
// Note: There is no notion of a cookie in the CHRE WiFi API so we need to use
// the global service state (gWifiServiceContext) directly in all callbacks.
struct ChppWifiServiceState gWifiServiceContext;

/************************************************
 *  Prototypes
 ***********************************************/

static bool chppDispatchWifiRequest(void *serviceContext, uint8_t *buf,
                                    size_t len);

static void chppWifiServiceOpen(struct ChppWifiServiceState *wifiServiceContext,
                                struct ChppAppHeader *requestHeader);
static void chppWifiServiceClose(
    struct ChppWifiServiceState *wifiServiceContext,
    struct ChppAppHeader *requestHeader);

static void chppWifiServiceGetCapabilities(
    struct ChppWifiServiceState *wifiServiceContext,
    struct ChppAppHeader *requestHeader);
static void chppWifiServiceConfigureScanMonitorAsync(
    struct ChppWifiServiceState *wifiServiceContext,
    struct ChppAppHeader *requestHeader, uint8_t *buf, size_t len);
static void chppWifiServiceRequestScanAsync(
    struct ChppWifiServiceState *wifiServiceContext,
    struct ChppAppHeader *requestHeader, uint8_t *buf, size_t len);
static void chppWifiServiceRequestRangingAsync(
    struct ChppWifiServiceState *wifiServiceContext,
    struct ChppAppHeader *requestHeader, uint8_t *buf, size_t len);

static void chppWifiServiceScanMonitorStatusChangeCallback(bool enabled,
                                                           uint8_t errorCode);
static void chppWifiServiceScanResponseCallback(bool pending,
                                                uint8_t errorCode);
static void chppWifiServiceScanEventCallback(struct chreWifiScanEvent *event);
static void chppWifiServiceRangingEventCallback(
    uint8_t errorCode, struct chreWifiRangingEvent *event);

/************************************************
 *  Private Functions
 ***********************************************/

/**
 * Dispatches a client request from the transport layer that is determined to be
 * for the WiFi service.
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
static bool chppDispatchWifiRequest(void *serviceContext, uint8_t *buf,
                                    size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;
  buf += sizeof(struct ChppAppHeader);
  len -= sizeof(struct ChppAppHeader);

  struct ChppWifiServiceState *wifiServiceContext =
      (struct ChppWifiServiceState *)serviceContext;
  bool success = true;

  switch (rxHeader->command) {
    case CHPP_WIFI_OPEN: {
      chppServiceTimestampRequest(&wifiServiceContext->open, rxHeader);
      chppWifiServiceOpen(wifiServiceContext, rxHeader);
      break;
    }

    case CHPP_WIFI_CLOSE: {
      chppServiceTimestampRequest(&wifiServiceContext->close, rxHeader);
      chppWifiServiceClose(wifiServiceContext, rxHeader);
      break;
    }

    case CHPP_WIFI_GET_CAPABILITIES: {
      chppServiceTimestampRequest(&wifiServiceContext->getCapabilities,
                                  rxHeader);
      chppWifiServiceGetCapabilities(wifiServiceContext, rxHeader);
      break;
    }

    case CHPP_WIFI_CONFIGURE_SCAN_MONITOR_ASYNC: {
      chppServiceTimestampRequest(
          &wifiServiceContext->configureScanMonitorAsync, rxHeader);
      chppWifiServiceConfigureScanMonitorAsync(wifiServiceContext, rxHeader,
                                               buf, len);
      break;
    }

    case CHPP_WIFI_REQUEST_SCAN_ASYNC: {
      chppServiceTimestampRequest(&wifiServiceContext->requestScanAsync,
                                  rxHeader);
      chppWifiServiceRequestScanAsync(wifiServiceContext, rxHeader, buf, len);
      break;
    }

    case CHPP_WIFI_REQUEST_RANGING_ASYNC: {
      chppServiceTimestampRequest(&wifiServiceContext->requestRangingAsync,
                                  rxHeader);
      chppWifiServiceRequestRangingAsync(wifiServiceContext, rxHeader, buf,
                                         len);
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
 * Initializes the WiFi service upon an open request from the client and
 * responds to the client with the result.
 *
 * @param serviceContext Maintains status for each service instance.
 * @param requestHeader App layer header of the request.
 */
static void chppWifiServiceOpen(struct ChppWifiServiceState *wifiServiceContext,
                                struct ChppAppHeader *requestHeader) {
  struct ChppAppHeader *response =
      chppAllocServiceResponseFixed(requestHeader, struct ChppAppHeader);

  static const struct chrePalWifiCallbacks palCallbacks = {
      .scanMonitorStatusChangeCallback =
          chppWifiServiceScanMonitorStatusChangeCallback,
      .scanResponseCallback = chppWifiServiceScanResponseCallback,
      .scanEventCallback = chppWifiServiceScanEventCallback,
      .rangingEventCallback = chppWifiServiceRangingEventCallback,
  };

  if (!wifiServiceContext->api->open(
          wifiServiceContext->service.appContext->systemApi, &palCallbacks)) {
    CHPP_LOGE("WiFi PAL API initialization failed");
    CHPP_DEBUG_ASSERT(false);
    response->error = CHPP_APP_ERROR_UNSPECIFIED;
  } else {
    response->error = CHPP_APP_ERROR_NONE;
  }

  chppSendTimestampedResponseOrFail(&wifiServiceContext->service,
                                    &wifiServiceContext->open, response,
                                    sizeof(*response));
}

/**
 * Deinitializes the WiFi service.
 *
 * @param serviceContext Maintains status for each service instance.
 * @param requestHeader App layer header of the request.
 */
static void chppWifiServiceClose(
    struct ChppWifiServiceState *wifiServiceContext,
    struct ChppAppHeader *requestHeader) {
  struct ChppAppHeader *response =
      chppAllocServiceResponseFixed(requestHeader, struct ChppAppHeader);

  wifiServiceContext->api->close();

  response->error = CHPP_APP_ERROR_NONE;
  chppSendTimestampedResponseOrFail(&wifiServiceContext->service,
                                    &wifiServiceContext->close, response,
                                    sizeof(*response));
}

/**
 * Retrieves a set of flags indicating the WiFi features supported by the
 * current implementation.
 *
 * @param serviceContext Maintains status for each service instance.
 * @param requestHeader App layer header of the request.
 */
static void chppWifiServiceGetCapabilities(
    struct ChppWifiServiceState *wifiServiceContext,
    struct ChppAppHeader *requestHeader) {
  struct ChppWifiGetCapabilitiesResponse *response =
      chppAllocServiceResponseFixed(requestHeader,
                                    struct ChppWifiGetCapabilitiesResponse);

  response->capabilities = wifiServiceContext->api->getCapabilities();
  response->header.error = CHPP_APP_ERROR_NONE;

  CHPP_LOGD("chppWifiServiceGetCapabilities returning %" PRIx32 ", %zu bytes",
            response->capabilities, sizeof(*response));

  chppSendTimestampedResponseOrFail(&wifiServiceContext->service,
                                    &wifiServiceContext->getCapabilities,
                                    response, sizeof(*response));
}

/**
 * Configures whether scanEventCallback receives unsolicited scan results, i.e.
 * the results of scans not performed at the request of CHRE.
 *
 * This function returns an error code synchronously. A subsequent call to
 * chppWifiServiceScanMonitorStatusChangeCallback() will be used to communicate
 * the result of the operation
 *
 * @param serviceContext Maintains status for each service instance.
 * @param requestHeader App layer header of the request.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppWifiServiceConfigureScanMonitorAsync(
    struct ChppWifiServiceState *wifiServiceContext,
    struct ChppAppHeader *requestHeader, uint8_t *buf, size_t len) {
  enum ChppAppErrorCode error = CHPP_APP_ERROR_NONE;

  if (len < sizeof(bool)) {
    error = CHPP_APP_ERROR_INVALID_ARG;
  } else {
    bool *enable = (bool *)buf;
    if (!wifiServiceContext->api->configureScanMonitor(*enable)) {
      error = CHPP_APP_ERROR_UNSPECIFIED;
    }
  }

  if (error != CHPP_APP_ERROR_NONE) {
    // Error occurred, send a synchronous error response
    struct ChppAppHeader *response =
        chppAllocServiceResponseFixed(requestHeader, struct ChppAppHeader);
    response->error = error;
    chppSendTimestampedResponseOrFail(
        &wifiServiceContext->service,
        &wifiServiceContext->configureScanMonitorAsync, response,
        sizeof(*response));
  }
}

/**
 * Request that the WiFi chipset perform a scan, or deliver results from its
 * cache if the parameters allow for it.
 *
 * This function returns an error code synchronously. A subsequent call to
 * chppWifiServiceScanEventCallback() will be used to communicate the result
 * of the operation
 *
 * @param serviceContext Maintains status for each service instance.
 * @param requestHeader App layer header of the request.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppWifiServiceRequestScanAsync(
    struct ChppWifiServiceState *wifiServiceContext,
    struct ChppAppHeader *requestHeader, uint8_t *buf, size_t len) {
  enum ChppAppErrorCode error = CHPP_APP_ERROR_NONE;

  // TODO: replace with chppWifiScanParamsToChre() when available
  if (len < sizeof(struct chreWifiScanParams)) {
    error = CHPP_APP_ERROR_INVALID_ARG;

  } else {
    struct ChppWifiScanParams *in = (struct ChppWifiScanParams *)buf;
    struct chreWifiScanParams *params = (struct chreWifiScanParams *)buf;

    if (in->frequencyListLen > 0 &&
        len >= in->frequencyList.offset +
                   in->frequencyListLen * sizeof(uint32_t)) {
      params->frequencyList =
          (const uint32_t *)(buf + in->frequencyList.offset);
    } else {
      params->frequencyList = NULL;
    }

    if (in->ssidListLen > 0 &&
        len >= in->ssidList.offset +
                   in->ssidListLen * sizeof(struct chreWifiSsidListItem)) {
      params->ssidList =
          (const struct chreWifiSsidListItem *)(buf + in->ssidList.offset);
    } else {
      params->ssidList = NULL;
    }

    if (!wifiServiceContext->api->requestScan(params)) {
      error = CHPP_APP_ERROR_UNSPECIFIED;
    }
  }

  if (error != CHPP_APP_ERROR_NONE) {
    // Error occurred, send a synchronous error response
    struct ChppAppHeader *response =
        chppAllocServiceResponseFixed(requestHeader, struct ChppAppHeader);
    response->error = error;
    chppSendTimestampedResponseOrFail(&wifiServiceContext->service,
                                      &wifiServiceContext->requestScanAsync,
                                      response, sizeof(*response));
  }
}

/**
 * Request that the WiFi chipset perform RTT ranging against a set of access
 * points specified in params.
 *
 * This function returns an error code synchronously. A subsequent call to
 * chppWifiServiceRangingEventCallback() will be used to communicate the
 * result of the operation
 *
 * @param serviceContext Maintains status for each service instance.
 * @param requestHeader App layer header of the request.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppWifiServiceRequestRangingAsync(
    struct ChppWifiServiceState *wifiServiceContext,
    struct ChppAppHeader *requestHeader, uint8_t *buf, size_t len) {
  enum ChppAppErrorCode error = CHPP_APP_ERROR_NONE;

  if (len < sizeof(struct chreWifiRangingParams)) {
    error = CHPP_APP_ERROR_INVALID_ARG;
  } else {
    struct chreWifiRangingParams *params = (struct chreWifiRangingParams *)buf;
    if (!wifiServiceContext->api->requestRanging(params)) {
      error = CHPP_APP_ERROR_UNSPECIFIED;
    }
  }

  if (error != CHPP_APP_ERROR_NONE) {
    // Error occurred, send a synchronous error response
    struct ChppAppHeader *response =
        chppAllocServiceResponseFixed(requestHeader, struct ChppAppHeader);
    response->error = error;
    chppSendTimestampedResponseOrFail(&wifiServiceContext->service,
                                      &wifiServiceContext->requestRangingAsync,
                                      response, sizeof(*response));
  }
}

/**
 * PAL callback with the result of changes to the scan monitor registration
 * status requested via configureScanMonitor.
 *
 * @param enabled true if the scan monitor is currently active
 * @param errorCode An error code from enum chreError
 */
static void chppWifiServiceScanMonitorStatusChangeCallback(bool enabled,
                                                           uint8_t errorCode) {
  // Recreate request header
  struct ChppAppHeader requestHeader = {
      .handle = gWifiServiceContext.service.handle,
      .transaction = gWifiServiceContext.configureScanMonitorAsync.transaction,
      .command = CHPP_WIFI_CONFIGURE_SCAN_MONITOR_ASYNC,
  };

  struct ChppWifiConfigureScanMonitorAsyncResponse *response =
      chppAllocServiceResponseFixed(
          &requestHeader, struct ChppWifiConfigureScanMonitorAsyncResponse);

  if (response == NULL) {
    CHPP_LOG_OOM();
    CHPP_ASSERT(false);

  } else {
    response->enabled = enabled;
    response->errorCode = errorCode;

    chppSendTimestampedResponseOrFail(
        &gWifiServiceContext.service,
        &gWifiServiceContext.configureScanMonitorAsync, response,
        sizeof(struct ChppWifiConfigureScanMonitorAsyncResponse));
  }
}

/**
 * PAL callback with the result of a requestScan.
 *
 * @param pending true if the request was successful.
 * @param errorCode An error code from enum chreError.
 */
static void chppWifiServiceScanResponseCallback(bool pending,
                                                uint8_t errorCode) {
  // Recreate request header
  struct ChppAppHeader requestHeader = {
      .handle = gWifiServiceContext.service.handle,
      .transaction = gWifiServiceContext.requestScanAsync.transaction,
      .command = CHPP_WIFI_REQUEST_SCAN_ASYNC,
  };

  struct ChppWifiRequestScanResponse *response = chppAllocServiceResponseFixed(
      &requestHeader, struct ChppWifiRequestScanResponse);

  if (response == NULL) {
    CHPP_LOG_OOM();
    CHPP_ASSERT(false);

  } else {
    response->pending = pending;
    response->errorCode = errorCode;

    chppSendTimestampedResponseOrFail(
        &gWifiServiceContext.service, &gWifiServiceContext.requestScanAsync,
        response, sizeof(struct ChppWifiRequestScanResponse));
  }
}

/**
 * PAL callback with WiFi scan result.
 *
 * @param event Scan result data.
 */
static void chppWifiServiceScanEventCallback(struct chreWifiScanEvent *event) {
  // Craft response per parser script
  struct ChppWifiScanEventWithHeader *notification;
  size_t notificationLen;
  if (!chppWifiScanEventFromChre(event, &notification, &notificationLen)) {
    CHPP_LOGE(
        "chppWifiScanEventFromChre failed (OOM?). Transaction ID = "
        "%" PRIu8,
        gWifiServiceContext.requestScanAsync.transaction);
    // TODO: consider sending an error response if this fails

  } else {
    notification->header.handle = gWifiServiceContext.service.handle;
    notification->header.type = CHPP_MESSAGE_TYPE_SERVICE_NOTIFICATION;
    notification->header.transaction =
        gWifiServiceContext.requestScanAsync.transaction;
    notification->header.error = CHPP_APP_ERROR_NONE;
    notification->header.command = CHPP_WIFI_REQUEST_SCAN_ASYNC;

    chppEnqueueTxDatagramOrFail(
        gWifiServiceContext.service.appContext->transportContext, notification,
        notificationLen);
  }

  gWifiServiceContext.api->releaseScanEvent(event);
}

/**
 * PAL callback with RTT ranging results from the WiFi module.
 *
 * @param errorCode An error code from enum chreError.
 * @param event Ranging data.
 */
static void chppWifiServiceRangingEventCallback(
    uint8_t errorCode, struct chreWifiRangingEvent *event) {
  // TODO

  UNUSED_VAR(errorCode);
  gWifiServiceContext.api->releaseRangingEvent(event);
}

/************************************************
 *  Public Functions
 ***********************************************/

void chppRegisterWifiService(struct ChppAppState *appContext) {
  gWifiServiceContext.api = chrePalWifiGetApi(CHRE_PAL_WIFI_API_V1_2);

  if (gWifiServiceContext.api == NULL) {
    CHPP_LOGE(
        "WiFi PAL API version not compatible with CHPP. Cannot register WiFi "
        "service");
    CHPP_DEBUG_ASSERT(false);

  } else {
    gWifiServiceContext.service.appContext = appContext;
    gWifiServiceContext.service.handle = chppRegisterService(
        appContext, (void *)&gWifiServiceContext, &kWifiServiceConfig);
    CHPP_DEBUG_ASSERT(gWifiServiceContext.service.handle);
  }
}

void chppDeregisterWifiService(struct ChppAppState *appContext) {
  // TODO

  UNUSED_VAR(appContext);
}